import json

import nirvana_api.json_rpc as json_rpc

REQUEST_ID = "2385fc2d-0d67-470b-b4a1-68ca8aa4b381"
JSONRPC = "2.0"
METHOD = "aaa"
RESULT = "bbb"


class ClassWithBuiltinAttributesNaming(object):

    def __init__(self):
        self.from_ = "2017-09-04"
        self.to = "2017-09-05"


def get_etalon_request(params, request_id=REQUEST_ID, json_rpc=JSONRPC, method=METHOD):
    return '{{"params": {}, "jsonrpc": "{}", "id": "{}", "method": "{}"}}'.format(
        json.dumps(params), json_rpc, request_id, method)


def get_etalon_response(result, request_id=REQUEST_ID, json_rpc=JSONRPC):
    return '{{"result": {}, "jsonrpc": "{}", "id": "{}"}}'.format(json.dumps(result), json_rpc, request_id)


def test_to_json():
    params = dict(wId='bbb', name='ccc')
    true_json = get_etalon_request(params)
    result_json = json_rpc.serialize(METHOD, params, REQUEST_ID)
    assert result_json == true_json


def test_to_str():
    true_json = get_etalon_response(RESULT)
    obj = json_rpc.deserialize(true_json)
    assert obj == RESULT


def test_to_int():
    result = 2
    true_json = get_etalon_response(result)
    obj = json_rpc.deserialize(true_json)
    assert obj == result


def test_to_str_list():
    result = ['progress', 'aaa', 'message', 'ccc']
    true_json = get_etalon_response(result)
    obj = json_rpc.deserialize(true_json)
    assert obj == result


def test_to_obj_list():
    true_json = get_etalon_response([{"progress": "aaa", "message": "ccc"}])
    obj = json_rpc.deserialize(true_json)
    assert len(obj) == 1
    assert obj[0].message == 'ccc'
    assert obj[0].progress == 'aaa'


def test_to_obj():
    true_json = get_etalon_response({"progress": "aaa", "message": "ccc"})
    obj = json_rpc.deserialize(true_json)
    assert obj.message == 'ccc'
    assert obj.progress == 'aaa'


def test_serialize():
    parameters_object = ClassWithBuiltinAttributesNaming()
    true_json = get_etalon_request({"from": parameters_object.from_, "to": parameters_object.to})
    result_json = json_rpc.serialize(METHOD, parameters_object, REQUEST_ID)
    assert result_json == true_json
