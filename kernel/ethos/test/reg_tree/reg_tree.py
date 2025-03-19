#import yatest.common
#import pytest

from datetime import datetime

import json
import requests

import time

basicWorkflow = 'aee19ae5-f57b-492d-80f9-7828c03e04dd'
regTreeBlockId = 'reg_tree_binary'

def RobustPost(url, params = None, files = None):
    headers = {"Authorization": "OAuth AQAD-qJSJhMaAAACj3qlkEj66kMpu1CiTgY1S3s"}
    for i in range(5):
        if params:
            resp = requests.post(url, headers=headers, json=params)
        if files:
            resp = requests.post(url, headers=headers, files=files)
        if resp.status_code == 200:
            return resp
        print resp.json()
        time.sleep(1)
    return None

def DeprecateData(data_id):
    api_host = 'https://nirvana.yandex-team.ru'
    api_deprecate = '/api/public/v1/deprecateData'
    url_deprecate = api_host + api_deprecate

    deprecate_params = {
        "jsonrpc" : "2.0",
        "id" : "id",
        "method": "deprecateData",
        "params" : {
            "dataId" : data_id,
            "params" : {
                "kind": "mandatory"
            }
        }
    }

    return RobustPost(url_deprecate, params=deprecate_params)

def UploadExecutable(filePath, dataName):
    api_host = 'https://nirvana.yandex-team.ru'

    api_create = '/api/public/v1/createData'
    api_upload = '/api/public/v1/uploadData'
    url_create = api_host + api_create
    url_upload = api_host + api_upload

    create_params = {
        "jsonrpc" : "2.0",
        "id" : "id",
        "method": "createData",
        "params" : {
            "name" : dataName,
            "description" : "",
            "type" : "executable",
        },
    }

    resp = RobustPost(url_create, params=create_params)
    if resp == None or not 'result' in resp.json():
        return None

    data_id = resp.json()["result"]

    upload_params = {
        "jsonrpc" : "2.0",
        "id" : "id",
        "method": "uploadData",
        "params" : {
            "dataId" : data_id,
            "params" : {
                'protocol': 'sync_http',
                "archive" : False,
            }
        }
    }

    files = {
        'content': ('content',open(filePath).read(), 'application/octet-stream', {}),
        'json_params': ('json', json.dumps(upload_params), 'application/json', {})
    }

    resp = RobustPost(url_upload, files=files)
    if resp == None or not 'result' in resp.json():
        return None
    return data_id

def CloneWorkflow():
    api_host = 'https://nirvana.yandex-team.ru'
    api_clone = '/api/public/v1/cloneWorkflow'
    url_clone = api_host + api_clone

    clone_params = {
        "jsonrpc" : "2.0",
        "id" : "id",
        "method": "cloneWorkflow",
        "params" : {
            "workflowId": basicWorkflow
        }
    }

    resp = RobustPost(url_clone, params=clone_params)
    if resp == None or not 'result' in resp.json():
        return None
    return resp.json()['result']

def ChangeData(workflowId, dataId):
    api_host = 'https://nirvana.yandex-team.ru'
    api_change = '/api/public/v1/editDataBlocks'
    url_change = api_host + api_change

    change_params = {
        "jsonrpc" : "2.0",
        "id" : "id",
        "method": "editDataBlocks",
        "params" : {
            "workflowId" : workflowId,
            "blocks" : [
                {
                    "code" : regTreeBlockId
                }
            ],
            "dataBlockParams" : {
                "dataId" : dataId
            }
        }
    }
    resp = RobustPost(url_change, params=change_params)

    if resp == None or not 'result' in resp.json():
        return None
    return str(resp.json()['result'])

def StartWorkflow(workflowId):
    api_host = 'https://nirvana.yandex-team.ru'
    api_start = '/api/public/v1/startWorkflow'
    url_start = api_host + api_start

    start_params = {
        "jsonrpc" : "2.0",
        "id" : "id",
        "method": "startWorkflow",
        "params" : {
            "workflowId" : workflowId
        }
    }

    resp = RobustPost(url_start, params=start_params)
    if resp == None or not 'result' in resp.json():
        return None
    return resp.json()['result']

def WorkflowExecutionState(workflowId):
    api_host = 'https://nirvana.yandex-team.ru'
    api_get_execution_state = '/api/public/v1/getExecutionState'
    url_get_execution_state = api_host + api_get_execution_state

    get_execution_state_params = {
        "jsonrpc" : "2.0",
        "id" : "id",
        "method": "getExecutionState",
        "params" : {
            "workflowId" : workflowId
        }
    }

    resp = RobustPost(url_get_execution_state, params=get_execution_state_params)
    if resp == None:
        return None

    respResult = resp.json()['result']

    resultParts = []

    if 'progress' in respResult:
        resultParts.append("%.0lf%% done" % (respResult['progress'] * 100))
        if 'message' in respResult:
            resultParts.append(respResult['message'])
    resultParts.append(respResult['status'])
    resultParts.append(respResult['result'])

    return resultParts

regTreeDataId = UploadExecutable('/home/alex-sh/bin/reg_tree', 'sample_reg_tree')
if regTreeDataId == None:
    print "nirvana is dead"
    exit(1)
print "reg_tree: " + regTreeDataId

workFlowId = CloneWorkflow()
if workFlowId == None:
    DeprecateData(regTreeDataId)
    print "nirvana is dead"
    exit(1)
print "workflow: " + workFlowId

changeDataResult = ChangeData(workFlowId, regTreeDataId)
if changeDataResult == None:
    DeprecateData(regTreeDataId)
    print "nirvana is dead"
    exit(1)
print "change data: " + changeDataResult

startWorkflow = StartWorkflow(workFlowId)
DeprecateData(regTreeDataId)
if startWorkflow == None:
    print "nirvana is dead"
    exit(1)
print "start workflow: " + startWorkflow

while 1:
    results = WorkflowExecutionState(workFlowId)
    print str(datetime.now()) + '\t' + '\t'.join(results)

    if results[-1] != 'undefined':
        break

    time.sleep(10)

