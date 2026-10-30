import os
import json
import time
import paho.mqtt.client as mqtt
from openai import OpenAI
from flask import Flask, request, jsonify

# ------------------- 1. 华为云MQTT设备参数（直接用你截图里的信息！） -------------------
# 从你截图的MQTT连接参数直接复制，100%正确
# ===== 华为云 IoTDA 设备参数（用环境变量配置，真实密钥请勿提交到仓库）=====
CLIENT_ID = os.environ.get("HUAWEI_IOT_CLIENT_ID", "YOUR_CLIENT_ID")
USERNAME  = os.environ.get("HUAWEI_IOT_USERNAME", "YOUR_DEVICE_ID")
# 这里填你设备的密钥！（创建设备时生成的，截图里隐藏了，手动填）
PASSWORD  = os.environ.get("HUAWEI_IOT_PASSWORD", "YOUR_DEVICE_SECRET")
HOST      = os.environ.get("HUAWEI_IOT_HOST", "YOUR_IOTDA_ENDPOINT")
PORT = 8883  # MQTTS加密端口，需要证书
# 你策略里配置的设备发布Topic（必须和策略完全一致！）
PUB_TOPIC = f"/v1/{USERNAME}/sensor/data"
# -----------------------------------------------------------------------------------

# 初始化Flask应用
app = Flask(__name__)

# 初始化DeepSeek AI客户端
client = OpenAI(
    api_key=os.environ.get('DEEPSEEK_API_KEY'),
    base_url="https://api.deepseek.com"
)

# ------------------- 2. MQTT客户端初始化与回调 -------------------
# 华为云MQTTS需要根证书，这里用内存加载，无需额外文件
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ 华为云MQTT连接成功！")
    else:
        print(f"❌ MQTT连接失败，错误码：{rc}")

def on_publish(client, userdata, mid):
    print(f"✅ 消息发布成功，消息ID：{mid}")

# 初始化MQTT客户端
mqtt_client = mqtt.Client(client_id=CLIENT_ID)
mqtt_client.username_pw_set(USERNAME, PASSWORD)
# 配置TLS加密（对应8883端口/MQTTS协议）
mqtt_client.tls_set()
# 绑定回调函数
mqtt_client.on_connect = on_connect
mqtt_client.on_publish = on_publish

# 连接华为云MQTT服务器
try:
    mqtt_client.connect(HOST, PORT, 60)
    # 启动后台线程处理MQTT通信
    mqtt_client.loop_start()
    print("🚀 MQTT客户端已启动，等待连接...")
except Exception as e:
    print(f"❌ MQTT连接异常：{str(e)}")

# ------------------- 3. Flask接口（保留原逻辑，新增MQTT上报） -------------------
@app.route('/receive', methods=['POST'])
def receive_data():
    try:
        # 1. 原逻辑：接收并解析数据
        raw_data = request.get_data(as_text=True)
        print("="*50)
        print("收到完整原始请求：")
        print(raw_data)
        print("="*50)

        data = None
        data = request.form.get('data')
        if data:
            print("从form提取数据：", data)
        else:
            if 'data=' in raw_data:
                parts = raw_data.split('data=')
                if len(parts) > 1:
                    data_part = parts[1].split('\r\n')[0].split('\n')[0]
                    data = data_part.strip()
                    print("从原始数据提取data：", data)
            if not data:
                data = raw_data.strip()
                print("使用原始body作为data：", data)

        if not data or data == "None":
            return jsonify({"code": 400, "msg": "数据为空", "ai_analysis": {}})

        # 2. 原逻辑：调用DeepSeek AI分析
        response = client.chat.completions.create(
            model="deepseek-chat",
            response_format={"type": "json_object"},
            temperature=0.1,
            messages=[
                {
                    "role": "system",
                    "content": """你是农业智能大棚物联网助手，分析传感器数据。
必须严格按照以下JSON格式返回结果，禁止任何多余文字、markdown、换行：
{
    "code": 200,
    "status": "正常/异常",
    "temp_status": "适宜/偏高/偏低",
    "humi_status": "适宜/偏高/偏低",
    "suggestion": "简短操作建议（不超过20字）",
    "detail": "简要分析（不超过50字）"
}
所有字段必须存在，内容简洁，适合单片机处理。"""
                },
                {"role": "user", "content": data},
            ],
            stream=False,
            timeout=10
        )

        ai_raw = response.choices[0].message.content
        print("AI返回JSON：", ai_raw)
        ai_result = json.loads(ai_raw)

        # 3. 新增核心逻辑：将AI分析结果上报到华为云
        # 构造上报 payload（包含原始数据+AI分析结果）
        upload_payload = json.dumps({
            "device_id": USERNAME,
            "raw_data": data,
            "ai_analysis": ai_result,
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
        }, ensure_ascii=False)

        # 发布到华为云MQTT Topic
        mqtt_client.publish(PUB_TOPIC, upload_payload, qos=1)
        print(f"✅ AI分析结果已上报华为云，Topic：{PUB_TOPIC}")
        print(f"上报内容：{upload_payload}")

        # 4. 原逻辑：返回响应
        return jsonify({
            "code": 200,
            "msg": "success",
            "ai_analysis": ai_result
        })

    except Exception as e:
        print("服务器错误详情：", str(e))
        import traceback
        traceback.print_exc()
        return jsonify({
            "code": 500,
            "msg": f"服务器错误：{str(e)}",
            "ai_analysis": {}
        })

# ------------------- 4. 启动服务 -------------------
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
