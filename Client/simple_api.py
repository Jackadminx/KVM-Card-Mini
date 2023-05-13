import json

from flask import Flask, request
from flask import jsonify

import hid
import time

vendor_id_s = 0x2b86
usage_page_s = 0xffb1
# vendor_id = 11142
# usage_page = 65457


def init_usb(vendor_id, usage_page):
    global h
    h = hid.device()
    hid_enumerate = hid.enumerate()
    device_path = 0
    # print(hid_enumerate)
    for i in range(len(hid_enumerate)):
        if (hid_enumerate[i]['usage_page'] == usage_page and hid_enumerate[i]['vendor_id'] == vendor_id):
            device_path = hid_enumerate[i]['path']
    if (device_path == 0): return "Device not found"
    h.open_path(device_path)
    h.set_nonblocking(1)  # enable non-blocking mode


def read_report(vendor_id, usage_page, buffer):
    print("------")
    print("<", buffer)
    time_start = time.time()
    try:
        h.write(buffer)
    except (OSError, ValueError):
        print("写入设备错误")
        return 1
    except NameError:
        print("未初始化设备")
        return 4
    while 1:
        try:
            d = h.read(64)
        except (OSError, ValueError):
            print("读取数据错误")
            return 2
        if d:
            print(">", d)
            break
        if time.time() - time_start > 3:
            d = 3
            break
    print("------")
    return d


# 新建一个应用
app = Flask(__name__)


# 路由（请求处理）
@app.route('/', methods=['GET'])
def hello():
    return jsonify({"msg": "hello world!"})


# 初始化设备
@app.route('/device_init', methods=['POST'])
def device_init():
    global vendor_id_s
    global usage_page_s
    print(request.data.decode())
    data = json.loads(request.data.decode())
    try:
        vendor_id = data['vendor_id']
        usage_page = data['usage_page']
    except KeyError:
        vendor_id = vendor_id_s
        usage_page = usage_page_s

    # print(vendor_id, usage_page)
    report = init_usb(vendor_id, usage_page)

    if report == "Device not found":
        return jsonify({"msg": "device not found"})
    else:
        vendor_id_s = vendor_id
        usage_page_s = usage_page
        return jsonify({"msg": "ok"})


# 关闭设备
@app.route('/device_close', methods=['GET'])
def device_close():
    try:
        h.close()
    except:
        return jsonify({"msg": "close error"})
    return jsonify({"msg": "closed"})


# 写入数据
@app.route('/hid_report', methods=['POST'])
def hid_report():
    data = json.loads(request.data.decode())
    try:
        buffer = data['report']
    except KeyError:
        return jsonify({"msg": "report not found"})

    try:
        vendor_id = data['vendor_id']
        usage_page = data['usage_page']
    except KeyError:
        vendor_id = vendor_id_s
        usage_page = usage_page_s

    if not isinstance(buffer, list):
        return jsonify({"msg": "report is not list. example: [4, 3]"})

    print(vendor_id, usage_page, buffer)

    report = read_report(vendor_id, usage_page, buffer)

    if report == 1:
        return jsonify({"msg": "write error"})
    elif report == 2:
        return jsonify({"msg": "read error"})
    elif report == 3:
        return jsonify({"msg": "timeout"})
    elif report == 4:
        return jsonify({"msg": "check initialized device"})
    else:
        return jsonify({"msg": "ok"}, {"report": report})


if __name__ == '__main__':
    # 运行参数
    app.run(host='0.0.0.0', port=8001, debug=True)
    h.close()
