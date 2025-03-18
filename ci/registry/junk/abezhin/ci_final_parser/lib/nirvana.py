# -*- coding: utf-8 -*-

import json
import logging
import os
import requests
import time
from requests.packages.urllib3.exceptions import InsecureRequestWarning

LOGGER = logging.getLogger('NirvanaWorkflow')


class Workflow:
    '''Common class for clone, update & execute nirvana workflow instance (graph)
    wrapper for api from https://wiki.yandex-team.ru/nirvana/Components/API/
    '''

    NIRVANA_API = 'https://nirvana.yandex-team.ru/api/public/v1/'

    def __init__(self, robot_nirvana_token, workflow_id, base_requests_id=None):
        self.robot_nirvana_token = robot_nirvana_token
        self.workflow_id = workflow_id
        self.base_requests_id = base_requests_id
        if self.base_requests_id is None:
            self.base_requests_id = '{}'.format(time.time())
        self.requests_id_cnt = 0

    def _request_headers(self):
        return {
            'Authorization': 'OAuth ' + self.robot_nirvana_token,
            'Content-type': 'application/json',
        }

    def _request_data(self, workflow_instance_id, method=None):
        self.requests_id_cnt += 1
        data = {
            'jsonrpc': '2.0',
            'id': '{}-{}'.format(self.base_requests_id, self.requests_id_cnt),
            'params': {
                'workflowId': self.workflow_id,
                'workflowInstanceId': workflow_instance_id,
            },
        }
        if method:
            data['method'] = method
        return data

    def _get_result(self, r, operation_name):
        LOGGER.info('{} http result code={}'.format(operation_name, r.status_code))
        if r.status_code != 200:
            raise Exception('{} failed: code={}'.format(operation_name, r.status_code))

        j_result = r.json()
        LOGGER.debug('{} http response={}'.format(operation_name, j_result))
        result = j_result.get('result')
        if result is None:
            message = j_result.get('error', {}).get('message')
            if message:
                raise Exception('{} failed: {}'.format(operation_name, message))
            raise Exception('{} failed: cannot get result field from response (json)'.format(operation_name))

        return result

    def gui_url(self, workflow_instance_id):
        return 'https://nirvana.yandex-team.ru/flow/{}/{}/graph'.format(self.workflow_id, workflow_instance_id)

    def clone_instance(self, workflow_instance_id):
        method = 'cloneWorkflowInstance'
        data = self._request_data(workflow_instance_id, method)
        LOGGER.info('clone_instance request_id={}'.format(data['id']))

        requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
        r = requests.get(
            self.NIRVANA_API + method,
            headers=self._request_headers(),
            json=json.dumps(data),
            params=data['params'],
            verify=False,
        )
        new_instance = self._get_result(r, 'clone_instance')  # workflowInstanceId of the new instance
        LOGGER.info('clone instance as {}'.format(self.gui_url(new_instance)))
        return new_instance

    def update_instance_block(self, workflow_instance_id, params):
        method = 'setBlockParameters'
        data = self._request_data(workflow_instance_id, method)
        LOGGER.info('update_instance_block id:{}'.format(data['id']))
        data['params'].update(params)

        requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
        r = requests.post(
            self.NIRVANA_API + method,
            headers=self._request_headers(),
            json=data,
            verify=False,
        )
        return self._get_result(r, 'update_instance_block')

    def update_instance_global_params(self, workflow_instance_id, params):
        method = 'setGlobalParameters'
        data = self._request_data(workflow_instance_id, method)
        LOGGER.info('update_instance_global_param id:{}'.format(data['id']))
        if isinstance(params, dict):
            data['params']['params'] = [{'parameter': k, 'value': v} for k, v in params.items()]
        elif isinstance(params, list):
            data['params']['params'] = params
        else:
            raise Exception('invalid params type (expected list or dict)')

        requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
        r = requests.post(
            self.NIRVANA_API + method,
            headers=self._request_headers(),
            json=data,
            verify=False,
        )
        return self._get_result(r, 'update_instance_global_params')

    def start_instance(self, workflow_instance_id):
        method = 'startWorkflow'
        data = self._request_data(workflow_instance_id, method)
        LOGGER.info('start_instance id:{}'.format(data['id']))

        requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
        r = requests.get(
            self.NIRVANA_API + method,
            headers=self._request_headers(),
            json=data,
            params=data['params'],
            verify=False,
        )
        return self._get_result(r, 'start_instance')

    def instance_status(self, workflow_instance_id):
        '''return tuple(status, result, progress)
        status: one of waiting/running/completed/undefined
        result: one of success/failure/cancel/undefined
        progress: from range [0.0, 1.0]
        '''
        method = 'getWorkflowSummary'
        data = self._request_data(workflow_instance_id, method)
        LOGGER.info('instance_status id:{}'.format(data['id']))
        requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
        r = requests.post(
            self.NIRVANA_API + method,
            headers=self._request_headers(),
            json=data,
            verify=False,
        )

        result = self._get_result(r, 'instance_status')
        return (result.get('status'), result.get('result'), result.get('progress'))

    def get_block_results(self, workflow_instance_id, block_name, block_endpoint):
        method = 'getBlockResults'
        data = self._request_data(workflow_instance_id, method)
        LOGGER.info('get_blocks_results id:{}'.format(data['id']))
        data['params']['blocks'] = [
            {
                'code': block_name,
            }
        ]

        requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
        r = requests.post(
            self.NIRVANA_API + method,
            headers=self._request_headers(),
            json=data,
            verify=False,
        )

        operation_name = 'get_block_results'
        LOGGER.info('{} http result code={}'.format(operation_name, r.status_code))
        if r.status_code != 200:
            raise Exception('{} failed: code={}'.format(operation_name, r.status_code))

        result = r.json()
        LOGGER.debug('{} http response={}'.format(operation_name, result))
        results_wrapper = result.get('result')
        target_to_download = None
        if results_wrapper:
            for wrapped_result in results_wrapper:
                inner_results = wrapped_result.get('results')
                if inner_results:
                    for item in inner_results:
                        endpoint = item.get('endpoint')
                        if endpoint and (endpoint == block_endpoint):
                            direct_path = item.get('directStoragePath')
                            if direct_path:
                                target_to_download = direct_path

        if not target_to_download:
            raise Exception(
                'get_block_results failed: not found results for workflow:{} block:{} output:{}'.format(
                    workflow_instance_id, block_name, block_endpoint
                )
            )

        requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
        r = requests.get(
            target_to_download,
            verify=False,
        )
        if r.status_code != 200:
            raise Exception('get_block_results failed: can not load result from url:{}'.format(target_to_download))

        return r.text


if __name__ == "__main__":
    root_logger = logging.getLogger('')
    out_logger = logging.StreamHandler()
    out_logger.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(filename)s:%(lineno)d %(message)s'))
    root_logger.addHandler(out_logger)
    root_logger.setLevel(logging.DEBUG)

    user_token = os.getenv('NIRVANA_TOKEN')
    if not user_token:
        logging.info('not found env. var. NIRVANA_TOKEN, try read token from ~/.nirvana-token')
        with open(os.path.join(os.getenv('HOME'), '.nirvana-token')) as f:
            user_token = f.read().strip()
    if not user_token:
        raise Exception('can not work without nirvana token (use env.var. or ~/.nirvana-token file)')

    logging.info('got token')
    workflow_id = 'acb103c8-7812-4af4-8b14-5b035f3f3f1d'
    workflow_instance_id = 'ef08089c-2b57-4874-b764-dc36b0f9d4dc'

    #
    # code for debug (+trivial example)
    #
    wf = Workflow(user_token, workflow_id)
    instance_copy = wf.clone_instance(workflow_instance_id)
    wf.update_instance_global_params(
        instance_copy,
        [
            {
                'parameter': 'test_global',
                'value': 'test_value',
            },
        ],
    )
    wf.update_instance_global_params(
        instance_copy,
        {'test_global2': 'test_value2'},
    )
    wf.update_instance_block(
        instance_copy,
        params={
            'blocks': [
                {
                    'code': 'operation-11',
                },
            ],
            'params': [
                {
                    'parameter': 'lines',
                    'value': ['test lines', 'line2'],
                },
            ],
        },
    )
    wf.start_instance(instance_copy)
    for i in range(1, 100):
        time.sleep(10)
        status = wf.instance_status(instance_copy)
        if status[0] == 'completed':
            if status[1] != 'success':
                raise Exception('workflow failed: {}'.format(wf.gui_url(instance_copy)))

            result = wf.get_block_results(instance_copy, 'operation-11', 'text')
            logging.info('workflow result: {}'.format(result))
            exit(0)

        logging.info('status/result/progress={}'.format(status))

    raise Exception('workflow failed: exhausted retry counter: {}'.format(wf.gui_url(instance_copy)))
