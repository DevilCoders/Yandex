from flask import (
    Flask,
    request,
    render_template,
)
import os
import json
import base64
import traceback
import random


SECRET_KEY = os.urandom(32)
root_path = os.getenv("ROOT_PATH", os.getcwd())
app = Flask(__name__, root_path=root_path, static_url_path='', static_folder=os.path.join(root_path, 'generated'))
app.config['SECRET_KEY'] = SECRET_KEY
mapping = None


@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        fingerprint = request.form.get('fingerprint')
        result = {}
        try:
            data = json.loads(fingerprint)

            for long_key, short_key in mapping.items():
                result[long_key] = data.pop(short_key, None)

            for short_key, value in data.items():
                result[short_key] = value

            return json.dumps(result, indent=4)
        except:
            traceback.print_exc()
            return 'Bad request'
    elif request.method == 'GET':
        return render_template('index.html')


def generate_d():
    # Ключ - это закодированный в base64 массив из 32 байт
    return base64.b64encode(bytes([random.randint(0, 255) for _ in range(32)])).decode()


def decode(rdata, d):
    # Для обфускации используем циклический XOR данных на ключ
    d = base64.b64decode(d)
    data_encoded = base64.b64decode(rdata)
    res = ''
    for i in range(len(data_encoded)):
        res += str(chr(data_encoded[i] ^ d[i % len(d)]))
    return res


@app.route('/hard', methods=['GET', 'POST'])
def hard():
    if request.method == 'POST':
        result = {}
        try:
            fingerprint = decode(request.form.get('rdata'), request.form.get('d'))
            data = json.loads(fingerprint)

            for long_key, short_key in mapping.items():
                result[long_key] = data.get(short_key, None)
            return json.dumps(result, indent=4)
        except:
            traceback.print_exc()
            return 'Bad request'
    elif request.method == 'GET':
        d = generate_d()
        return render_template('hard.html', d=d)


def main():
    global mapping
    with open(os.path.join(root_path, "generated/mapping.json")) as mapping_file:
        mapping = json.load(mapping_file)
    app.run(debug=False, host='::', port=12312)


if __name__ == '__main__':
    main()
