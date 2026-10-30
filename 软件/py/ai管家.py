import os
import json
import time
import paho.mqtt.client as mqtt
from openai import OpenAI
from flask import Flask, request, jsonify
import threading
import requests


# ------------------- 1. 华为云MQTT设备参数 -------------------
# ===== 华为云 IoTDA 设备参数（用环境变量配置，真实密钥请勿提交到仓库）=====
CLIENT_ID = os.environ.get("HUAWEI_IOT_CLIENT_ID", "YOUR_CLIENT_ID")
USERNAME  = os.environ.get("HUAWEI_IOT_USERNAME", "YOUR_DEVICE_ID")
PASSWORD  = os.environ.get("HUAWEI_IOT_PASSWORD", "YOUR_DEVICE_SECRET")
HOST      = os.environ.get("HUAWEI_IOT_HOST", "YOUR_IOTDA_ENDPOINT")
PORT = 8883
PUB_TOPIC = "/test/M2M/from_pyduan"
SUB_TOPIC = "/test/M2M/from_appduan"

ESP32_URL = os.environ.get("ESP32_URL", "http://192.168.1.100/control")
# -----------------------------------------------------------------------------------
# 全局变量：保存最新传感器数据（给AI判断使用）
latest_sensor_data = {}
last_report_time = 0          # 上次上报时间
REPORT_INTERVAL = 30          # 1分钟上报一次（60秒）
last_reported_data = None  # 上次上报的数据
CHANGE_THRESHOLD = 5   # 变化超过 5% 就判定为大变化
CHANGE_THRESHOLDCO2 = 100   # 变化超过 5% 就判定为大变化

Ai_advice = ""

app = Flask(__name__)

ai_client = OpenAI(
    api_key=os.environ.get('DEEPSEEK_API_KEY'),
    base_url="https://api.deepseek.com"
)

# ------------------- 2. MQTT客户端初始化 -------------------
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ 华为云MQTT连接成功！")
        client.subscribe(SUB_TOPIC, qos=1)
        print("✅ 已订阅APP指令主题：" + SUB_TOPIC)
    else:
        print(f"❌ MQTT连接失败，错误码：{rc}")

# ==============================
# 🔥 核心：接收APP指令 + AI防误触
# ==============================
def on_message(client, userdata, msg):
    global Ai_advice  # 声明要使用全局变量
    try:
        payload = msg.payload.decode('utf-8')
        print("\n" + "=" * 60)
        print("📥 【Python 收到 APP 指令】")
        print("指令内容：", payload)

        cmd_data = json.loads(payload)
        cmd_type = cmd_data.get("cmd")
        ai_check = cmd_data.get("ai_check", 0)

        # ==============================================
        # 1. 设备控制指令（水泵、水雾）
        # ==============================================
        if cmd_type == "control":
            pump = cmd_data.get("pump", 0)
            fog = cmd_data.get("fog", 0)

            print("🔴 设备控制：水泵=%d, 水雾=%d | AI校验=%d" % (pump, fog, ai_check))

            # --------------------------
            # AI防误触开启
            # --------------------------
            if ai_check == 1:
                print("⏳ AI 正在分析环境是否需要开启...")

                if not latest_sensor_data:
                    print("⚠️ 暂无传感器数据，返回拦截")
                    ai_result = "block"
                else:
                    data_str = json.dumps(latest_sensor_data, ensure_ascii=False)
                    prompt = f"""
                                环境：{data_str}
                                操作：水泵{pump} 水雾{fog}
                                规则：香蕉25-30℃,湿度60-80%,土湿40-60%,根据环境：{data_str}{Ai_advice}是否允许或拦截设备水泵{pump} 水雾{fog}开启
                                请以 JSON 格式输出，仅返回如下结构
                                输出：{{"ai_result":"allow/block"}}
                              """

                    response = ai_client.chat.completions.create(
                        model="deepseek-chat",
                        response_format={"type": "json_object"},
                        temperature=0.1,
                        messages=[{"role": "user", "content": prompt}],
                        max_tokens=20,  # 限制输出
                        stream=False,
                        timeout=15
                    )

                    ai_resp = json.loads(response.choices[0].message.content)
                    ai_result = ai_resp.get("ai_result", "block")

                print(f"🤖 AI判断结果：{ai_result}")

                # 🔥 发送结果给APP（格式完全匹配）
                mqtt_client.publish(PUB_TOPIC, json.dumps({
                    "device_id": USERNAME,
                    "ai_result": ai_result,
                    "timestamp": time.strftime("%Y-%m-%d %H:%M:%S")
                }, ensure_ascii=False), qos=1)

                # 执行设备控制
                if ai_result == "allow":
                    send_control_to_esp32(pump, fog)
                    print("✅ AI允许开启，已执行控制")
                else:
                    print("❌ AI拦截开启，不执行")

            # --------------------------
            # AI关闭：直接执行
            # --------------------------
            else:
                send_control_to_esp32(pump, fog)
                print("✅ AI关闭，直接执行控制")

        # ==============================================
        # 2. 阈值设置
        # ==============================================
        elif cmd_type == "threshold":
            print("🟢 阈值设置指令")
            send_threshold_to_esp32(cmd_data)

        # ==============================================
        # 3. 自动模式
        # ==============================================
        elif cmd_type == "auto":
            print("🟢 切换自动模式")

        else:
            print("⚫ 未知指令")

        print("=" * 60 + "\n")

    except Exception as e:
        print("❌ 指令处理错误：", e)

def on_publish(client, userdata, mid):
    print(f"✅ 消息发布成功，消息ID：{mid}")

mqtt_client = mqtt.Client(client_id=CLIENT_ID)
mqtt_client.username_pw_set(USERNAME, PASSWORD)
mqtt_client.tls_set()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.on_publish = on_publish

try:
    mqtt_client.connect(HOST, PORT, 60)
    mqtt_client.loop_start()
    print("🚀 MQTT客户端已启动")
except Exception as e:
    print(f"❌ MQTT连接异常：{str(e)}")

# ------------------- 工具方法：发送原始数据到云端 -------------------
def send_raw_data_to_cloud(data):
    try:
        payload = json.dumps({
            "device_id": USERNAME,
            "raw_data": data,
            "ai_analysis": {},
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S")
        }, ensure_ascii=False)
        mqtt_client.publish(PUB_TOPIC, payload, qos=1)
        print("✅【实时】传感器数据已上报云端")
    except Exception as e:
        print("❌ 发送原始数据失败：", e)

# ------------------- AI分析 + 发送结果 -------------------
def ai_analyze_and_send(data):
    global Ai_advice  # 声明要使用全局变量
    try:
        ai_response = '''
        你是香蕉大棚助手。按要求输出JSON，无多余内容：
            {
                "code": 200,
                "status": "正常/异常",
                "temp_status": "适宜/偏高/偏低",
                "humi_status": "适宜/偏高/偏低",
                "ai_advice": "水泵作用增加土壤湿度和水雾用于空气降温或者增加空气湿度，给建议时加上是否需要开启水泵或水雾≤30字",
                "detail": "简要分析,重点预测植物生长环境（≤80字）"
            }
            所有字段必须存在，内容简洁，适合单片机处理.
        '''

        print("⏳ 后台开始环境AI分析...")
        response = ai_client.chat.completions.create(
            model="deepseek-chat",
            response_format={"type": "json_object"},
            temperature=0.1,
            messages=[
                {
                    "role": "system",
                    "content": ai_response
                },
                {"role": "user", "content": data},
            ],
            stream=False,
            timeout=10
        )

        ai_result = json.loads(response.choices[0].message.content)
        print("✅ AI分析完成：", ai_result)

        # 提取 ai_advice 字段
        Ai_advice = ai_result["ai_advice"]

        # 🔥 第二次发送：带AI结果
        payload = json.dumps({
            "device_id": USERNAME,
            "raw_data": data,
            "ai_analysis": ai_result,
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S")
        }, ensure_ascii=False)

        mqtt_client.publish(PUB_TOPIC, payload, qos=1)
        print("✅【后台】环境AI分析已上报")

    except Exception as e:
        print("❌ AI分析失败：", e)

# ------------------- 接收ESP32数据 -------------------
@app.route('/receive', methods=['POST'])
def receive_data():
    global latest_sensor_data, last_reported_data, last_report_time
    temp_change = 0
    humi_change = 0
    soil_change = 0
    ph_change = 0
    CO2_change = 0
    li_change = 0
    try:
        raw_data = request.get_data(as_text=True)
        print("=" * 50)
        print("收到原始数据：", raw_data)
        print("=" * 50)

        if request.form.get('data'):
            data = request.form.get('data')
        elif 'data=' in raw_data:
            data = raw_data.split('data=')[1].split('\r\n')[0].strip()
        else:
            data = raw_data.strip()

        if not data or data == "None":
            return jsonify({"code": 400, "msg": "数据为空", "ai_analysis": {}})

        # 保存最新传感器/转成JSON
        current_data = json.loads(data)
        latest_sensor_data = current_data  # 永远保存最新数据
        now = time.time()

        # ==============================================
        # 🔥 核心逻辑：数据是否变化很大？
        # ==============================================
        need_report = False
        if last_reported_data is None:
            need_report = True
        else:
            #计算数据变化值
            temp_chage = abs(current_data["temp_value_2"] - last_reported_data["temp_value_2"])
            humi_change = abs(current_data["kq_humi_value_2"] - last_reported_data["kq_humi_value_2"])
            soil_change = abs(current_data["t_humi_value"] - last_reported_data["t_humi_value"])
            ph_change = abs(current_data["t_ph_value"] - last_reported_data["t_ph_value"])
            CO2_change = abs(current_data["CO2_value"] - last_reported_data["CO2_value"])
            li_change = abs(current_data["light"] - last_reported_data["light"])

            # 任意一个变化 > 5，立即上报
            if temp_change > CHANGE_THRESHOLD or humi_change > CHANGE_THRESHOLD or soil_change > CHANGE_THRESHOLD or ph_change > CHANGE_THRESHOLD or li_change > CHANGE_THRESHOLD or CO2_change > CHANGE_THRESHOLDCO2:
                print("✅ 数据变化较大，立即上报！")
                need_report = True
            elif now - last_report_time >= REPORT_INTERVAL:
                print("✅ 1分钟到，定时上报")
                need_report = True

        # ==============================================
        # 执行上报 + AI
        # ==============================================
        if need_report:
            #立刻发原始数据
            send_raw_data_to_cloud(data)
            # 后台线程跑AI，不卡住主程序
            threading.Thread(target=ai_analyze_and_send, args=(data,)).start()
            # 更新最后上报时间
            last_report_time = now
            # 更新上次上报的数据
            last_reported_data = current_data

        # 立刻返回，不用等AI
        return jsonify({
            "code": 200,
            "msg": "数据已实时上报，AI分析中...",
            "ai_analysis": {}
        })

    except Exception as e:
        print("错误：", str(e))
        return jsonify({"code": 500, "msg": "服务器错误"})

# ------------------- 发送控制指令到ESP32 -------------------
def send_control_to_esp32(pump, fog):
    try:
        data = {"cmd": "control", "pump": pump, "fog": fog}
        requests.post(ESP32_URL, json=data, timeout=3)
        print("✅ 控制指令已发送 → ESP32 → STM32")
    except Exception as e:
        print("❌ 发送 ESP32 失败：请检查IP是否正确", e)

def send_threshold_to_esp32(threshold_data):
    try:
        requests.post(ESP32_URL, json=threshold_data, timeout=3)
        print("✅ 阈值指令已发送 → ESP32 → STM32")
    except Exception as e:
        print("❌ 发送 ESP32 失败：请检查IP是否正确", e)

# ------------------- 启动 -------------------
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True,use_reloader=False)
