import os
import json
from openai import OpenAI
from flask import Flask, request, jsonify
#启动一个 Web 服务器
app = Flask(__name__)

client = OpenAI(
    api_key=os.environ.get('DEEPSEEK_API_KEY'),
    base_url="https://api.deepseek.com"
)
#定义接口路由 /receive，创建一个 POST 接口
@app.route('/receive', methods=['POST'])
def receive_data():
    try:
        # 1. 先打印完整原始请求，方便调试
        raw_data = request.get_data(as_text=True)
        print("="*50)
        print("收到完整原始请求：")
        print(raw_data)
        print("="*50)

        # 2. 多方式提取data，彻底解决None问题
        data = None
        # 方式1：优先从form获取（标准格式）
        data = request.form.get('data')
        if data:
            print("从form提取数据：", data)
        else:
            # 方式2：从原始数据中提取data=xxx
            if 'data=' in raw_data:
                # 分割出data=后面的内容，直到\r\n或结束
                parts = raw_data.split('data=')
                if len(parts) > 1:
                    data_part = parts[1].split('\r\n')[0].split('\n')[0]
                    data = data_part.strip()
                    print("从原始数据提取data：", data)
            # 方式3：如果都没有，直接用整个body作为data
            if not data:
                data = raw_data.strip()
                print("使用原始body作为data：", data)

        # 3. 空数据校验
        if not data or data == "None":
            return jsonify({"code": 400, "msg": "数据为空", "ai_analysis": {}})

        # 4. 调用AI分析
        response = client.chat.completions.create(
            model="deepseek-chat",
            response_format={"type": "json_object"},
            temperature=0.1,  # 降低随机性，保证JSON稳定
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

        return jsonify({
            "code": 200,
            "msg": "success",
            "ai_analysis": ai_result
        })

    except Exception as e:
        print("服务器错误详情：", str(e))
        import traceback
        traceback.print_exc()  # 打印完整错误栈，方便调试
        return jsonify({
            "code": 500,
            "msg": f"服务器错误：{str(e)}",
            "ai_analysis": {}
        })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)