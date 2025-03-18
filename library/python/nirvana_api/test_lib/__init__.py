import sys
import json
import random
import re
import uuid

import nirvana_api as nv


CLEANUP_RULES = [
    (r"\"(id|code|remotePath|operationId)\": \"[^\"]*\"", "\"\\1\": \"<removed-value>\""),
]
CLEANUP_RULES_KEEP_CODE = [
    (r"\"(id|remotePath|operationId)\": \"[^\"]*\"", "\"\\1\": \"<removed-value>\""),
]


def apply_cleanup_rules(line, rules=CLEANUP_RULES):
    for pattern, repl in rules:
        line = re.sub(pattern, repl, line)
    return line


def get_mock_response(method):
    if method in (
            'startWorkflow',
            'stopWorkflow',
            'createWorkflow',
            ):
        res = 'MOCKED_RESPONSE'
    elif method in (
            'getExecutionState',
            'getWorkflowSummary',
            'getDataUploadState',
            ):
        res = {
            "status": nv.ExecutionStatus.completed,
            "result": nv.ExecutionResult.success
        }
    elif method in (
            'getBlockResults',
            'storeBlockResults',
            'getBlockLogs',
            'findData',
            ):
        res = []
    elif method in (
            'invalidateBlockCache',
            'rejectBlockResults',
            ):
        res = 42
    else:
        res = {}
    return res


def block_code_name(guid, name, idx):
    return '{}_{}'.format(name, idx)


class BlockCodeMock(object):
    def __init__(self, code_func=block_code_name):
        self.global_index = 0
        self.code_func = code_func

    def get_block_code(self, guid, name):
        res = self.code_func(guid, name, self.global_index)
        self.global_index += 1
        return res


class NirvanaApiMock(nv.NirvanaApi):
    def __init__(self, *args, **kwargs):
        self.output = kwargs.pop('output', sys.stdout)
        self.line_modifier = kwargs.pop('line_modifier', None)
        self.response_mocker = kwargs.pop('response_mocker', get_mock_response)
        self.rand = random.Random(42)
        super(NirvanaApiMock, self).__init__("XXXXXXXX", *args[1:], **kwargs)

    def _make_uuid(self):
        return str(uuid.UUID(int=self.rand.randint(0, 2 ** 128 - 1)))

    def _do_batch_request(self, method, params):
        reqid = self._make_uuid()
        self.log_request(
            method, nv.json_rpc.serialize_batch(method, params, request_id=reqid))
        mock_response = self.response_mocker(method)
        batch_response = [
            {"result": mock_response, "id": "{id}-{idx}".format(id=reqid, idx=idx)}
            for idx in range(len(params))
        ]
        return nv.json_rpc.deserialize_batch(json.dumps(batch_response), reqid)

    def _do_request(self, method, params):
        self.log_request(
            method, nv.json_rpc.serialize(method, params, request_id=self._make_uuid()))
        mock_response = self.response_mocker(method)
        return nv.json_rpc.deserialize(json.dumps({"result": mock_response}))

    def _do_multipart_request(self, method, params, files):
        data = nv.json_rpc.serialize(method, params, request_id=self._make_uuid())
        files_arg = {'json_params': ('json', data, 'application/json', {})}
        file_obj = files['content'][1]
        files = {'content': ('content', file_obj.read(), 'application/octet-stream', {})}
        files_arg.update(files)
        self.log_request(method, json.dumps(files_arg))
        mock_response = self.response_mocker(method)
        return nv.json_rpc.deserialize(json.dumps({"result": mock_response}))

    def log_request(self, method, text):
        self.output.write("{0}(\n".format(method))
        js = json.loads(text)
        pretty_text = json.dumps(js, indent=2, sort_keys=True)
        if self.line_modifier is not None:
            lines = pretty_text.split('\n')
            pretty_text = '\n'.join(self.line_modifier(line) for line in lines)
        self.output.write("  " + pretty_text.replace("\n", "\n  "))
        self.output.write("\n)\n")
