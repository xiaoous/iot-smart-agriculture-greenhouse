import os
import json
import time
import paho.mqtt.client as mqtt
from openai import OpenAI
from flask import Flask, request, jsonify
import threading  # 🔥 新增：后台线程（不卡住主程序）
import requests  # 新增：用于HTTP转发给ESP32

# ------------------- 1. 华为云MQTT设备参数 -------------------
# ===== 华为云 IoTDA 设备参数（用环境变量配置，真实密钥请勿提交到仓库）=====
CLIENT_ID = os.environ.get("HUAWEI_IOT_CLIENT_ID", "YOUR_CLIENT_ID")
USERNAME  = os.environ.get("HUAWEI_IOT_USERNAME", "YOUR_DEVICE_ID")
PASSWORD  = os.environ.get("HUAWEI_IOT_PASSWORD", "YOUR_DEVICE_SECRET")
HOST      = os.environ.get("HUAWEI_IOT_HOST", "YOUR_IOTDA_ENDPOINT")
PORT = 8883
PUB_TOPIC = "/test/M2M/from_pyduan"
SUB_TOPIC = "/test/M2M/from_appduan"

# ESP32 HTTP 地址（你后面改成自己的）
ESP32_URL = os.environ.get("ESP32_URL", "http://192.168.1.100/control")
# -----------------------------------------------------------------------------------

app = Flask(__name__)

client = OpenAI(
    api_key=os.environ.get('DEEPSEEK_API_KEY'),
    base_url="https://api.deepseek.com"
)


# ------------------- 2. MQTT客户端初始化 -------------------
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ 华为云MQTT连接成功！")
        # ==============================
        # 🔥 新增：连接成功后订阅APP指令
        # ==============================
        client.subscribe(SUB_TOPIC, qos=1)
        print("✅ 已订阅APP指令主题：" + SUB_TOPIC)
    else:
        print(f"❌ MQTT连接失败，错误码：{rc}")


# ==============================
# 🔥 新增：接收APP指令的回调函数
# 自动识别：设备控制 / 阈值设置 / 自动模式
# ==============================
def on_message(client, userdata, msg):
    try:
        # 接收APP发来的指令数据
        payload = msg.payload.decode('utf-8')
        print("\n" + "=" * 60)
        print("📥 【Python 收到 APP 指令】")
        print("指令主题：", msg.topic)
        print("指令内容：", payload)

        # 解析JSON指令
        cmd_data = json.loads(payload)
        print("🔍 解析后指令：", cmd_data)

        # 获取指令类型
        cmd_type = cmd_data.get("cmd")

        # ==============================================
        # 1. 设备控制指令（水泵、水雾）
        # ==============================================
        if cmd_type == "control":
            pump = cmd_data.get("pump")  # 水泵 1=开 0=关
            fog = cmd_data.get("fog")  # 水雾 1=开 0=关

            print("🔴 指令类型：设备控制")
            print("⚡ 水泵：", "开启" if pump == 1 else "关闭")
            print("⚡ 水雾：", "开启" if fog == 1 else "关闭")

            # ==============================
            # 后续可直接调用：转发给ESP32
            send_control_to_esp32(pump, fog)
            # ==============================

        # ==============================================
        # 2. 阈值设置指令（6项参数）
        # ==============================================
        elif cmd_type == "threshold":
            print("🟢 指令类型：阈值设置")
            temp_max = cmd_data.get("temp_max")
            humi_min = cmd_data.get("humi_min")
            soil_humi_min = cmd_data.get("soil_humi_min")
            ph_max = cmd_data.get("ph_max")
            light_min = cmd_data.get("light_min")
            co2_max = cmd_data.get("co2_max")

            print("📊 温度最大值：", temp_max)
            print("📊 湿度最小值：", humi_min)
            print("📊 土壤湿度最小值：", soil_humi_min)
            print("📊 土壤酸碱度最大值：", ph_max)
            print("📊 光照最小值：", light_min)
            print("📊 二氧化碳最大值：", co2_max)

            # ==============================
            # 后续可直接调用：转发阈值给ESP32
            send_threshold_to_esp32(cmd_data)
            # ==============================

        # ==============================================
        # 3. 自动模式
        # ==============================================
        elif cmd_type == "auto":
            print("🟡 指令类型：切换为自动模式")

        # ==============================================
        # 未知指令
        # ==============================================
        else:
            print("⚫ 未知指令类型：", cmd_type)

        print("=" * 60 + "\n")

    except Exception as e:
        print("❌ 指令解析错误：", e)


def on_publish(client, userdata, mid):
    print(f"✅ 消息发布成功，消息ID：{mid}")


mqtt_client = mqtt.Client(client_id=CLIENT_ID)
mqtt_client.username_pw_set(USERNAME, PASSWORD)
mqtt_client.tls_set()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message  # 绑定消息回调
mqtt_client.on_publish = on_publish

try:
    mqtt_client.connect(HOST, PORT, 60)
    mqtt_client.loop_start()
    print("🚀 MQTT客户端已启动")
except Exception as e:
    print(f"❌ MQTT连接异常：{str(e)}")


# ------------------- 工具方法：发送数据到云端（立即发送） -------------------
def send_raw_data_to_cloud(data):
    try:
        # 🔥 只发原始数据，0延迟
        payload = json.dumps({
            "device_id": USERNAME,
            "raw_data": data,
            "ai_analysis": {},  # 先空着
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S")
        }, ensure_ascii=False)

        mqtt_client.publish(PUB_TOPIC, payload, qos=1)
        print("✅【实时】原始数据已发送到云端 → App秒显示")
    except Exception as e:
        print("❌ 发送原始数据失败：", e)


# ------------------- 工具方法：AI分析 + 发送AI结果（后台跑） -------------------
def ai_analyze_and_send(data):
    try:
        print("⏳ 后台开始AI分析...")

        # AI分析
        response = client.chat.completions.create(
            model="deepseek-chat",
            response_format={"type": "json_object"},
            temperature=0.1,
            messages=[
                {
                    "role": "system",
                    "content": """你是农业智能大棚物联网助手，种植的是香蕉，需要根据植物分析各种传感器数据。
        必须严格按照以下JSON格式返回结果，禁止任何多余文字、markdown、换行：
        {
            "code": 200,
            "status": "正常/异常",
            "temp_status": "适宜/偏高/偏低",
            "humi_status": "适宜/偏高/偏低",
            "ai_advice": "简短操作建议（种植的是香蕉，有水泵浇水和水雾降温，需要综合建议如果检测数据异常时提示香蕉适宜的环境数据给建议时需要建议开启水泵或水雾不超过30字）",
            "detail": "简要分析（种植的是香蕉不超过50字）"
        }
        所有字段必须存在，内容简洁，适合单片机处理。"""
                },
                {"role": "user", "content": data},
            ],
            stream=False,
            timeout=10
        )

        ai_raw = response.choices[0].message.content
        ai_result = json.loads(ai_raw)
        print("✅ AI分析完成：", ai_result)

        # 🔥 第二次发送：带AI结果
        payload = json.dumps({
            "device_id": USERNAME,
            "raw_data": data,
            "ai_analysis": ai_result,
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S")
        }, ensure_ascii=False)

        mqtt_client.publish(PUB_TOPIC, payload, qos=1)
        print("✅【后台】AI结果已发送到云端")

    except Exception as e:
        print("❌ AI分析失败：", e)


# ------------------- 主接口 -------------------
@app.route('/receive', methods=['POST'])
def receive_data():
    try:
        raw_data = request.get_data(as_text=True)
        print("=" * 50)
        print("收到原始数据：", raw_data)
        print("=" * 50)

        # 提取数据
        data = None
        if request.form.get('data'):
            data = request.form.get('data')
        elif 'data=' in raw_data:
            data = raw_data.split('data=')[1].split('\r\n')[0].strip()
        else:
            data = raw_data.strip()

        if not data or data == "None":
            return jsonify({"code": 400, "msg": "数据为空", "ai_analysis": {}})

        # ==============================================
        # 🔥 核心修改：1. 立刻发原始数据（0延迟）
        # ==============================================
        send_raw_data_to_cloud(data)

        # ==============================================
        # 🔥 核心修改：2. 后台线程跑AI，不卡住主程序
        # ==============================================
        threading.Thread(target=ai_analyze_and_send, args=(data,)).start()

        # 立刻返回，不用等AI
        return jsonify({
            "code": 200,
            "msg": "数据已实时上报，AI分析中...",
            "ai_analysis": {}
        })

    except Exception as e:
        print("错误：", str(e))
        return jsonify({"code": 500, "msg": "服务器错误", "ai_analysis": {}})


# ------------------- 【可选】发给ESP32的函数，你后面直接启用 -------------------
def send_control_to_esp32(pump, fog):
    try:
        data = {
            "cmd": "control",
            "pump": pump,
            "fog": fog
        }
        # 👇 把 data=data 改成 json=data，自动发送 JSON 格式
        requests.post(ESP32_URL, json=data, timeout=3)
        print("✅ 控制指令已发送 → ESP32 → STM32")
    except Exception as e:
        print("❌ 发送 ESP32 失败：请检查IP是否正确", e)

def send_threshold_to_esp32(threshold_data):
    try:
        # 👇 把 data=threshold_data 改成 json=threshold_data
        requests.post(ESP32_URL, json=threshold_data, timeout=3)
        print("✅ 阈值指令已发送 → ESP32 → STM32")
    except Exception as e:
        print("❌ 发送 ESP32 失败：请检查IP是否正确", e)


# ------------------- 启动 -------------------
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
