import base64
import json
import os
import random
import traceback
from collections import defaultdict, OrderedDict
from operator import itemgetter

from flask import (
    Flask,
    request,
    render_template,
)


SECRET_KEY = os.urandom(32)
FP_NAME_TO_FP_CODE = None

root_path = os.getenv("ROOT_PATH", os.getcwd())
app = Flask(__name__, root_path=root_path, static_url_path='', static_folder=os.path.join(root_path, 'generated'))
app.config['SECRET_KEY'] = SECRET_KEY


def _load_mapping():
    global FP_NAME_TO_FP_CODE
    with open(os.path.join(root_path, "generated/mapping.json")) as mapping_file:
        FP_NAME_TO_FP_CODE = json.load(mapping_file)
        FP_NAME_TO_FP_CODE.pop('version')


def _group_fingerprints(fp_code_to_fp_value):
    '''
    :param fp_code_to_fp_value: fingerprint data collected by greed.js
    :return: { "browser" : { "navigator" : [ "language" : "en", ... ], ... }, ... }
    '''
    groupped_fingerprints = defaultdict(lambda: defaultdict(list))

    for fp_name, fp_code in FP_NAME_TO_FP_CODE.items():
        groups = fp_name.split('_')

        if len(groups) == 2:
            # pad groups for shorter names, i.e. "headless_audiocontext"
            main_group, subgroup, name = groups[0], '', groups[1]
        else:
            main_group, subgroup, name = groups

        groupped_fingerprints[main_group][subgroup].append({
            'name': name,
            'value': fp_code_to_fp_value.get(fp_code, None)
        })

    # order groups alphabetically
    ordered_groupped_fingerprints = OrderedDict()

    for main_group, data in sorted(groupped_fingerprints.items()):
        ordered_groupped_fingerprints[main_group] = OrderedDict()

        for subgroup, names_and_values in sorted(data.items()):
            ordered_groupped_fingerprints[main_group][subgroup] = sorted(names_and_values, key=itemgetter('name'))

    return ordered_groupped_fingerprints


@app.route('/', methods=['GET', 'POST'])
def index():
    headers = [{
        'key': 'User Agent',
        'value': request.headers.get('User-Agent', '')
    }, {
        'key': 'ja3',
        'value': request.headers.get('X-Yandex-Ja3', '')
    }, {
        'key': 'ja4',
        'value': request.headers.get('X-Yandex-Ja4', '')
    }, {
        'key': 'Yandex-Trust token',
        'value': request.headers.get('Yandex-Trust', '')
    }, {
        'key': 'Yandex-Trust info',
        'value': request.headers.get('X-Antirobot-Yandex-Trust-Info', '')
    }]

    if request.method == 'POST':
        fp_json = json.loads(request.form.get('fingerprint'))
        fp_version = fp_json.pop('v', None)
        groupped_fingerprints = _group_fingerprints(fp_json)

        return render_template('fingerprints.html', headers=headers, groupped_fingerprints=groupped_fingerprints, version=fp_version)
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

            for fp_name, fp_code in FP_NAME_TO_FP_CODE.items():
                result[fp_name] = data.get(fp_code, None)
            return json.dumps(result, indent=4)
        except:
            traceback.print_exc()
            return 'Bad request'
    elif request.method == 'GET':
        d = generate_d()
        return render_template('hard.html', d=d)


def main():
    _load_mapping()
    app.run(debug=False, host='::', port=12312)


if __name__ == '__main__':
    main()
