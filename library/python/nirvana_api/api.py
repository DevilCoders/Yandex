import logging
import uuid

import requests
from requests.packages.urllib3 import Retry

from . import json_rpc
from .parameter_classes import DeprecateKind

logger = logging.getLogger(__name__)

# [doc types]
from .parameter_classes import PaginationData, WorkflowFilters  # noqa


try:
    xrange
except NameError:
    xrange = range

NIRVANA_HOST = 'nirvana.yandex-team.ru'
NIRVANA_API_PREFIX = 'api/public/v1'


class NirvanaApi(object):

    class Batch(object):

        class Result(object):

            def __init__(self):
                self.ready = False
                self.result = None

            def get(self):
                if not self.ready:
                    raise Exception('result not ready')
                return self.result

            def set(self, result):
                self.result = result
                self.ready = True

        def __init__(self, api):
            self.api = api
            self.requests = []
            self.method = None
            self.results = []

        def add(self, method, params):
            if self.method is not None and method != self.method:
                raise Exception('cant batch requests with deifferent methods')
            self.method = method
            self.requests.append(params)
            future = self.Result()
            self.results.append(future)
            return future

        def __enter__(self):
            self.api._batch = self
            return self

        def __exit__(self, evalue, etype, eframe):
            self.api._batch = None
            errors = []

            if not evalue and self.requests:
                chunk_size = self.api.max_batch_size
                num_chunks = (chunk_size - 1 + len(self.requests)) // chunk_size

                for chunk_index in xrange(num_chunks):
                    chunk_begin = chunk_index * chunk_size
                    chunk_end = min(chunk_begin + chunk_size, len(self.requests))

                    chunk_requests = self.requests[chunk_begin:chunk_end]
                    chunk_results = self.results[chunk_begin:chunk_end]

                    api_res = {o.sub_id: o for o in self.api._do_batch_request(self.method, chunk_requests)}

                    for idx, (request, future) in enumerate(zip(chunk_requests, chunk_results)):
                        if idx not in api_res:
                            errors.append((request, None, 'idx not found'))
                            continue

                        result = api_res[idx]
                        if hasattr(result, 'error'):
                            errors.append((request, result, None))
                        else:
                            future.set(result.result)

            for request, result, error_text in errors:
                logger.error('request method: {}'.format(self.method))
                logger.error('data header {}'.format(request))
                if result:
                    logger.error('Failed to deserialize {}'.format(result.error))
                    logger.error('response {}'.format(result))
                if error_text:
                    logger.error(error_text)

            for request, result, error_text in errors:
                if result:
                    raise json_rpc.RPCException(result.error.code, result.error.message, getattr(result.error, 'data', ''), request)
                if not error_text:
                    error_text = 'unknown error'
                raise json_rpc.RPCException(500, error_text, '', request)

    def __init__(
        self,
        oauth_token,
        server=None,
        api_prefix=NIRVANA_API_PREFIX,
        timeout=300,
        max_retries=10,
        ssl_verify=True,
        max_batch_size=100,
        pool_connections=10,
        pool_maxsize=10,
        method_whitelist=Retry.DEFAULT_METHOD_WHITELIST | {"POST"}
    ):
        logging.getLogger('requests').setLevel(logging.WARNING)
        self.server = server or NIRVANA_HOST
        self.url_prefix = self.server if self.server.startswith("http") else "https://" + self.server
        self.api_prefix = api_prefix
        self.oauth_token = oauth_token
        self.timeout = timeout
        self.ssl_verify = ssl_verify
        self._session = requests.Session()
        retries = Retry(total=max_retries, backoff_factor=1, status_forcelist=[429, 500, 502, 503, 504], method_whitelist=method_whitelist)
        adapter = requests.adapters.HTTPAdapter(pool_connections=pool_connections, pool_maxsize=pool_maxsize, max_retries=retries)
        self._session.mount('http://', adapter)
        self._session.mount('https://', adapter)
        self._batch = None
        self.max_batch_size = max_batch_size

    def batch(self):
        if self._batch:
            raise Exception('already in batch')
        return self.Batch(self)

    def _get_url(self, method):
        return '{}/{}/{}'.format(self.url_prefix, self.api_prefix, method)

    def _get_headers(self):
        return {'Authorization': 'OAuth {}'.format(self.oauth_token), 'Content-Type': 'application/json;charset=utf-8'}

    def _get_multipart_headers(self):
        return {'Authorization': 'OAuth {}'.format(self.oauth_token)}

    def _get_storage_headers(self):
        return {'Authorization': 'OAuth {}'.format(self.oauth_token)}

    def _format_request(self, url, headers, data):
        return 'Request url: {}\nHeaders: {}\nRequest body {}'.format(url, headers, data).replace(self.oauth_token, "***")

    @staticmethod
    def _format_response(response):
        return 'Response: {}'.format(response.text)

    def _do_batch_request(self, method, params):
        request_id = str(uuid.uuid4())
        data = json_rpc.serialize_batch(method, params, request_id=request_id)
        url = self._get_url(method)
        headers = self._get_headers()
        try:
            response = self._session.post(
                url,
                headers=headers,
                data=data,
                timeout=self.timeout,
                verify=self.ssl_verify
            )
        except Exception as e:
            logger.error('Failed to make request: {}'.format(e))
            logger.error(self._format_request(url, headers, data))
            raise e

        try:
            logger.log(NIRVANA_DEBUG_ALL, self._format_request(url, headers, data))
            logger.log(NIRVANA_DEBUG_ALL, self._format_response(response))
        except Exception as e:
            logger.error('Failed to log {}'.format(e))

        try:
            return json_rpc.deserialize_batch(response.text, base_request_id=request_id)
        except Exception as e:
            logger.error('Failed to deserialize {}'.format(e))
            logger.error(self._format_request(url, headers, data))
            logger.error(self._format_response(response))
            raise e

    def _request(self, method, params):
        if self._batch:
            return self._batch.add(method, params)
        return self._do_request(method, params)

    def _do_request(self, method, params):
        data = json_rpc.serialize(method, params)
        response = self._session.post(
            self._get_url(method),
            headers=self._get_headers(),
            data=data,
            timeout=self.timeout,
            verify=self.ssl_verify
        )
        try:
            logger.log(NIRVANA_DEBUG_ALL, 'Request body {}'.format(data))
            logger.log(NIRVANA_DEBUG_ALL, 'Response {}'.format(response.text))
        except Exception as e:
            logger.error('Failed to log {}'.format(e))
        try:
            response.raise_for_status()
            return json_rpc.deserialize(response.text)
        except Exception as e:
            logger.error('Failed to deserialize {}'.format(e))
            logger.error('request: {}'.format(self._get_url(method)))
            logger.error('request body {}'.format(data))
            logger.error('response {}'.format(response.text))
            logger.error('response.status_code {}'.format(response.status_code))
            logger.error('data header {}'.format(data))
            raise e

    def _do_multipart_request(self, method, params, files):
        data = json_rpc.serialize(method, params)
        files_arg = {'json_params': ('json', data, 'application/json', {})}
        files_arg.update(files)
        response = self._session.post(
            self._get_url(method),
            headers=self._get_multipart_headers(),
            files=files_arg,
            timeout=self.timeout,
            verify=self.ssl_verify
        )
        try:
            logger.log(NIRVANA_DEBUG_ALL, 'Request body {}'.format(data))
            logger.log(NIRVANA_DEBUG_ALL, 'Response {}'.format(response.text))
        except Exception as e:
            logger.error('Failed to log {}'.format(e))
        try:
            return json_rpc.deserialize(response.text)
        except Exception as e:
            logger.error('Failed to deserialize {}'.format(e))
            logger.error('request: {}'.format(self._get_url(method)))
            logger.error('response {}'.format(response.text))
            logger.error('data header {}'.format(data[0:400]))
            raise e

    @staticmethod
    def _update_options(options, workflow_id=None, workflow_instance_id=None):
        if workflow_id:
            options['workflowId'] = workflow_id
        if workflow_instance_id:
            options['workflowInstanceId'] = workflow_instance_id

    def get_username(self):
        return self._request('getOrCreateUser', dict())

    def get_from_storage(self, storage_url, stream=False):
        response = self._session.get(
            storage_url,
            headers=self._get_headers(),
            timeout=self.timeout,
            verify=self.ssl_verify,
            stream=stream
        )
        try:
            logger.log(NIRVANA_DEBUG_ALL, 'Request url {}'.format(storage_url))
            if not stream:
                logger.log(NIRVANA_DEBUG_ALL, 'Response length {}'.format(len(response.text)))
        except Exception as e:
            logger.error('Failed to log {}'.format(e))
        return response

    def get_workflow_url(self, workflow_id, workflow_instance_id=None):
        if workflow_instance_id:
            return '{0}/flow/{1}/{2}/graph'.format(self.url_prefix, workflow_id, workflow_instance_id)
        return '{0}/flow/{1}/graph'.format(self.url_prefix, workflow_id)

    def get_operation_url(self, operation_id):
        return '{0}/operation/{1}/overview'.format(self.url_prefix, operation_id)

    def get_block_url(self, block_guid, workflow_id=None, workflow_instance_id=None):
        if workflow_id:
            return '{0}/flow/{1}/graph/FlowchartBlockOperation/{2}'.format(self.url_prefix, workflow_id, block_guid)
        if workflow_instance_id:
            return '{0}/process/{1}/graph/FlowchartBlockOperation/{2}'.format(self.url_prefix, workflow_instance_id,
                                                                              block_guid)
        return None

    def get_data_url(self, data_id):
        return '{0}/data/{1}'.format(self.url_prefix, data_id)

    def start_workflow(self, workflow_id=None, workflow_instance_id=None):
        return self._request('startWorkflow', dict(workflowId=workflow_id, workflowInstanceId=workflow_instance_id))

    def stop_workflow(self, workflow_id=None, workflow_instance_id=None):
        return self._request(
            'stopWorkflow',
            dict(workflowId=workflow_id, workflowInstanceId=workflow_instance_id)
        )

    def subscribe_to_workflow(self, workflow_id, user, states=None):
        return self._request(
            'subscribeToWorkflow',
            dict(workflowId=workflow_id, userId=user, executionResults=states)
        )

    def unsubscribe_from_workflow(self, workflow_id, user):
        return self._request(
            'unsubscribeFromWorkflow',
            dict(workflowId=workflow_id, userId=user)
        )

    def get_subscriptions(self, workflow_id):
        return self._request(
            'getSubscriptions',
            dict(workflowId=workflow_id)
        )

    def get_execution_state(self, workflow_id=None, block_patterns=None, workflow_instance_id=None):
        return self._request(
            'getExecutionState',
            dict(workflowId=workflow_id, blocks=_list(block_patterns), workflowInstanceId=workflow_instance_id)
        )

    def get_workflow_execution_states(self, workflow_instance_ids):
        return self._request(
            'getWorkflowExecutionStates',
            dict(workflowInstanceIds=workflow_instance_ids)
        )

    def listen_workflow_instance_result(
        self, callback_url,
        workflow_id=None, workflow_instance_id=None, block=None, target_results=None,
    ):
        return self._request(
            'listenWorkflowInstanceResult',
            dict(
                callbackUrl=callback_url, workflowId=workflow_id, workflowInstanceId=workflow_instance_id,
                block=block, targetResults=target_results,
            )
        )

    def get_block_results(self, workflow_id=None, block_patterns=None, outputs=None, workflow_instance_id=None):
        return self._request(
            'getBlockResults',
            dict(
                workflowId=workflow_id,
                blocks=_list(block_patterns),
                outputs=_list(outputs),
                workflowInstanceId=workflow_instance_id
            )
        )

    def store_block_results(self, workflow_id=None, block_patterns=None, outputs=None, workflow_instance_id=None):
        return self._request(
            'storeBlockResults',
            dict(
                workflowId=workflow_id,
                blocks=_list(block_patterns),
                outputs=_list(outputs),
                workflowInstanceId=workflow_instance_id
            )
        )

    def get_block_logs(self, workflow_id=None, block_patterns=None, log_names=None, workflow_instance_id=None):
        return self._request(
            'getBlockLogs',
            dict(
                workflowId=workflow_id,
                blocks=_list(block_patterns),
                logNames=_list(log_names),
                workflowInstanceId=workflow_instance_id
            )
        )

    def invalidate_block_cache(self, workflow_id=None, block_patterns=None, workflow_instance_id=None):
        return self._request(
            'invalidateBlockCache',
            dict(workflowId=workflow_id, blocks=_list(block_patterns), workflowInstanceId=workflow_instance_id)
        )

    def reject_block_results(self, workflow_id=None, workflow_instance_id=None):
        return self._request(
            'rejectBlockResults',
            dict(workflowId=workflow_id, workflowInstanceId=workflow_instance_id)
        )

    def create_workflow(self, name, quota_project_id=None, project_code=None, description=None, ns_id=None, ns_path=None):
        params = dict(name=name, quotaProjectId=quota_project_id, projectCode=project_code)
        if description:
            params['description'] = description
        if ns_id:
            params['nsId'] = ns_id
        if ns_path:
            params['nsPath'] = ns_path
        return self._request('createWorkflow', params)

    def create_workflow_instance(self, workflow_id, workflow_instance=None):
        params = dict(workflowId=workflow_id)
        if workflow_instance:
            params['workflowInstance'] = workflow_instance
        return self._request('createWorkflowInstance', params)

    def write_workflow_instance(self, workflow_instance, workflow_instance_id=None):
        params = dict(workflowInstance=workflow_instance)
        if workflow_instance_id:
            params['workflowInstanceId'] = workflow_instance_id
        return self._request('writeWorkflowInstance', params)

    def clone_workflow(self, workflow_id=None, new_name=None, new_quota_project_id=None, new_project_code=None, workflow_instance_id=None, new_description=None, new_ns_id=None):
        options = dict(
            newName=new_name,
            newDescription=new_description,
            newQuotaProjectId=new_quota_project_id,
            newNsId=new_ns_id,
            newProjectCode=new_project_code
        )
        self._update_options(options, workflow_id, workflow_instance_id)
        return self._request('cloneWorkflow', options)

    def clone_workflow_instance(self, workflow_id=None, workflow_instance_id=None, target_workflow_id=None, new_quota_project_id=None):
        return self._request(
            'cloneWorkflowInstance',
            dict(workflowId=workflow_id, workflowInstanceId=workflow_instance_id, targetWorkflowId=target_workflow_id, newQuotaProjectId=new_quota_project_id)
        )

    def fork_workflow_instance(self, workflow_instance_id, new_name=None, new_description=None, new_quota_project_id=None, new_tags=None, strict_tag_check=None, new_ns_id=None):
        return self._request(
            'forkWorkflowInstance',
            dict(workflowInstanceId=workflow_instance_id, newName=new_name, newDescription=new_description,
                 newTags=new_tags, strictTagCheck=strict_tag_check,
                 newQuotaProjectId=new_quota_project_id, newNsId=new_ns_id)
        )

    def delete_workflow(self, workflow_id):
        return self._request('deleteWorkflow', dict(workflowId=workflow_id))

    def delete_workflow_instance(self, workflow_instance_id):
        return self._request('deleteWorkflowInstance', dict(workflowInstanceId=workflow_instance_id))

    def get_workflow(self, workflow_id=None, workflow_instance_id=None):
        return self._request(
            'getWorkflow',
            dict(workflowId=workflow_id, workflowInstanceId=workflow_instance_id)
        )

    def get_block_meta_data(self, workflow_id=None, workflow_instance_id=None, block_patterns=None, params=None):
        return self._request(
            'getBlockMetaData',
            dict(workflowId=workflow_id, workflowInstanceId=workflow_instance_id,
                 blocks=_list(block_patterns), params=_list(params))
        )

    def get_block_inputs(self, workflow_id=None, workflow_instance_id=None, block_patterns=None, inputs=None):
        return self._request(
            'getBlockInputs',
            dict(workflowId=workflow_id, workflowInstanceId=workflow_instance_id,
                 blocks=_list(block_patterns), inputs=_list(inputs))
        )

    def get_block_parameters(self, workflow_id=None, workflow_instance_id=None, block_patterns=None, params=None, return_nulls=False):
        return self._request(
            'getBlockParameters',
            dict(workflowId=workflow_id, workflowInstanceId=workflow_instance_id,
                 blocks=_list(block_patterns), params=_list(params), returnNulls=return_nulls)
        )

    def set_block_parameters(self, workflow_id=None, workflow_instance_id=None, block_patterns=None, params=None, strict_parameter_matching=None):
        options = dict(
            blocks=_list(block_patterns), params=_list(params), strictParameterMatching=strict_parameter_matching)
        self._update_options(options, workflow_id, workflow_instance_id)
        return self._request(
            'setBlockParameters',
            options
        )

    def link_block_parameters(self, workflow_id=None, workflow_instance_id=None, block_patterns=None, params=None, strict_parameter_matching=None):
        options = dict(
            blocks=_list(block_patterns), params=_list(params), strictParameterMatching=strict_parameter_matching)
        self._update_options(options, workflow_id, workflow_instance_id)
        return self._request(
            'linkBlockParameters',
            options
        )

    def add_global_parameters(self, workflow_id=None, workflow_instance_id=None, global_params=None):
        options = dict(
            params=_list(global_params)
        )
        self._update_options(options, workflow_id, workflow_instance_id)
        return self._request(
            'addGlobalParameters',
            options
        )

    def get_global_parameters(self, workflow_id=None, workflow_instance_id=None):
        options = {}
        self._update_options(options, workflow_id, workflow_instance_id)
        return self._request(
            'getGlobalParameters',
            options
        )

    def get_global_parameters_meta_data(self, workflow_id=None, workflow_instance_id=None):
        options = {}
        self._update_options(options, workflow_id, workflow_instance_id)
        return self._request(
            'getGlobalParametersMetaData',
            options
        )

    def set_global_parameters(self, workflow_id=None, workflow_instance_id=None, param_values=None):
        options = dict(
            params=_list(param_values)
        )
        self._update_options(options, workflow_id, workflow_instance_id)
        return self._request(
            'setGlobalParameters',
            options
        )

    def get_workflow_meta_data(self, workflow_id=None, workflow_instance_id=None):
        return self._request('getWorkflowMetaData', dict(workflowId=workflow_id, workflowInstanceId=workflow_instance_id))

    def get_workflow_summary(self, workflow_id=None, block_patterns=None, workflow_instance_id=None):
        return self._request(
            'getWorkflowSummary',
            dict(workflowId=workflow_id, blocks=_list(block_patterns), workflowInstanceId=workflow_instance_id)
        )

    def get_workflow_execution_params(self, workflow_id=None, workflow_instance_id=None):
        return self._request('getWorkflowExecutionParams', dict(workflowId=workflow_id, workflowInstanceId=workflow_instance_id))

    def find_workflows(self, pattern, pagination_data=None, workflow_filters=None):
        return self._request(
            'findWorkflows',
            dict(pattern=pattern, paginationData=pagination_data, additionalFilters=workflow_filters)
        )

    def validate_workflow(self, workflow_id=None, workflow_instance_id=None):
        return self._request('validateWorkflow', dict(workflowId=workflow_id, workflowInstanceId=workflow_instance_id))

    def approve_workflow(self, workflow_id=None, workflow_instance_id=None):
        return self._request('approveWorkflow', dict(workflowId=workflow_id, workflowInstanceId=workflow_instance_id))

    def edit_workflow(
        self,
        workflow_id=None,
        name=None,
        owner=None,
        description=None,
        quota_project_id=None,
        execution_params=None,
        permissions=None,
        tags=None,
        workflow_instance_id=None,
        main_instance_id=None,
        main_instance_auto_update=None
    ):
        options = {}
        options['workflowParams'] = dict(
            name=name,
            owner=owner,
            description=description,
            quotaProjectId=quota_project_id,
            executionSettings=execution_params,
            permissions=_permissions(permissions),
            tags=tags,
            mainInstanceId=main_instance_id,
            mainInstanceAutoUpdate=main_instance_auto_update
        )
        self._update_options(options, workflow_id, workflow_instance_id)
        return self._request('editWorkflow', options)

    def edit_workflow_tags(self, workflow_id=None, tags=None):
        return self._request(
            'editWorkflowTags',
            dict(workflowId=workflow_id, newTags=tags)
        )

    def edit_workflow_name_and_description(self, workflow_id, new_name=None, new_description=None):
        return self._request(
            'editWorkflowNameAndDescription',
            dict(
                workflowId=workflow_id,
                newName=new_name,
                newDescription=new_description,
            )
        )

    def add_workflow_instance_comment(self, workflow_instance_id, comment):
        return self._request('addCommentToWorkflowInstance', dict(workflowInstanceId=workflow_instance_id, comment=comment))

    def read_workflow_instance(self, workflow_instance_id):
        return self._request('readWorkflowInstance', dict(workflowInstanceId=workflow_instance_id))

    def create_data(self, name, data_type, description=None, destination=None, quota_project=None, ttl_days=None):
        return self._request(
            'createData',
            dict(name=name, type=data_type, description=description, destination=destination, quotaProject=quota_project, ttlDays=ttl_days)
        )

    def clone_data(self, data_id, new_name=None, new_description=None, new_destination=None, new_quota_project_id=None):
        return self._request(
            'cloneData',
            dict(
                dataId=data_id,
                newName=new_name,
                newDescription=new_description,
                newDestination=new_destination,
                newQuotaProjectId=new_quota_project_id,
            )
        )

    def edit_data_tags(self, data_id, tags=None):
        return self._request(
            'editDataTags',
            dict(
                dataId=data_id,
                newTags=tags
            )
        )

    def edit_data(self, data_id, name=None, data_type=None, description=None, quota_project=None, permissions=None):
        return self._request(
            'editData',
            dict(
                dataId=data_id,
                params=dict(
                    name=name,
                    dataType=data_type,
                    description=description,
                    quotaProject=quota_project,
                    permissions=_permissions(permissions),
                )
            )
        )

    def upload_data_multipart(self, data_id, upload_parameters, file, file_name='content'):
        return self._do_multipart_request(
            'uploadData',
            dict(dataId=data_id, params=upload_parameters),
            {'content': (file_name, file, 'application/octet-stream', {})}
        )

    def upload_data(self, data_id, upload_parameters):
        return self._request(
            'uploadData',
            dict(dataId=data_id, params=upload_parameters),
        )

    def get_data_upload_state(self, data_id):
        return self._request('getDataUploadState', dict(dataId=data_id))

    def get_data(self, data_id):
        return self._request('getData', dict(dataId=data_id))

    def find_data(self, pattern=None, storage_path=None, pagination_data=None, additional_filters=None):
        return self._request(
            'findData',
            dict(pattern=pattern, storagePath=storage_path,
                 paginationData=pagination_data, additionalFilters=additional_filters)
        )

    def deprecate_data(self, data_id, kind=DeprecateKind.optional, description=None, migration_hint=None, replaced_by=None):
        return self._request(
            'deprecateData',
            dict(
                dataId=data_id,
                params=dict(kind=kind, description=description, migrationHint=migration_hint, replacedBy=replaced_by)
            )
        )

    def clone_operation(self, operation_id, new_name=None, new_tags=None, strict_tag_check=None):
        return self._request(
            'cloneOperation',
            dict(
                operationId=operation_id,
                newName=new_name,
                newTags=new_tags,
                strictTagCheck=strict_tag_check
            )
        )

    def clone_operation_version_to_different_operation(self, version_id, new_name, new_tags=None, strict_tag_check=None, alias_code=None):
        return self._request(
            'cloneOperationVersionToDifferentOperation',
            dict(
                aliasCode=alias_code,
                versionId=version_id,
                newName=new_name,
                newTags=new_tags,
                strictTagCheck=strict_tag_check
            )
        )

    def set_alias_for_opeartion(self, operation_id, code, set_main):
        return self._request(
            'setAliasForOperation',
            dict(
                operationId=operation_id,
                code=code,
                setMain=set_main
            )
        )

    def get_operation(self, operation_id):
        return self._request('getOperation', dict(operationId=operation_id))

    def get_alias_code_by_version_id(self, version_id):
        return self._request('getAliasCodeByVersionId', dict(versionId=version_id))

    def read_operation(self, operation_id):
        return self._request('readOperation', dict(operationId=operation_id))

    def find_operation(self, pattern, include_deprecated=False, pagination_data=None, additional_filters=None):
        return self._request(
            'findOperation',
            dict(pattern=pattern, includeDeprecated=include_deprecated,
                 paginationData=pagination_data, additionalFilters=additional_filters)
        )

    def approve_operation(self, operation_id):
        return self._request('approveOperation', dict(operationId=operation_id))

    def validate_operation(self, operation_id):
        return self._request('validateOperation', dict(operationId=operation_id))

    def edit_operation_meta(self, operation_id, name=None, owner=None, description=None, version_number=None, deterministic=None, permissions=None, tags=None):
        return self._request(
            'editOperationMeta',
            dict(
                operationId=operation_id,
                params=dict(
                    name=name,
                    owner=owner,
                    description=description,
                    deterministic=deterministic,
                    versionNumber=version_number,
                    permissions=_permissions(permissions),
                    tags=tags
                )
            )
        )

    def edit_operation_name_and_description(self, operation_id, new_name=None, new_description=None):
        return self._request(
            'editOperationNameAndDescription',
            dict(operationId=operation_id, newName=new_name, newDescription=new_description)
        )

    def edit_operation_parameters(self, operation_id, params):
        return self._request('editOperationParameters', dict(operationId=operation_id, params=_list(params)))

    def edit_operation_tags(self, operation_id, tags):
        return self._request("editOperationTags", dict(operationId=operation_id, newTags=tags))

    def set_alias_for_operation(self, operation_id, alias, set_main=False):
        return self._request('setAliasForOperation', dict(operationId=operation_id, code=alias, setMain=set_main))

    def get_operation_by_alias(self, alias, version_number=None):
        return self._request(
            'getOperationByAlias',
            dict(
                code=alias,
                versionNumber=version_number
            )
        )

    def get_operation_import_process_by_alias_and_version(self, alias_code, version_number):
        return self._request(
            'getOperationImportProcessByAliasAndVersion',
            dict(aliasCode=alias_code, versionNumber=version_number)
        )

    def deprecate_operation(self, operation_id, kind=DeprecateKind.optional, description=None, migration_hint=None, replaced_by=None):
        return self._request(
            'deprecateOperation',
            dict(
                operationId=operation_id,
                params=dict(kind=kind, description=description, migrationHint=migration_hint, replacedBy=replaced_by)
            )
        )

    def delete_operation(self, operation_id):
        return self._request('deleteOperation', dict(operationId=operation_id))

    def get_operation_endpoints(self, operation_id):
        return self._request('getOperationEndpoints', dict(operationId=operation_id))

    def add_operation_endpoints(self, operation_id, operation_endpoint=None):
        return self._request('addOperationEndpoints', dict(operationId=operation_id, endpoints=_list(operation_endpoint)))

    def remove_operation_endpoints(self, operation_id, operation_endpoint_references=None):
        return self._request('removeOperationEndpoints', dict(operationId=operation_id, endpoints=_list(operation_endpoint_references)))

    def get_operation_parameters(self, operation_id):
        return self._request('getOperationParameters', dict(operationId=operation_id))

    def add_operation_parameters(self, operation_id, operation_parameters=None):
        return self._request('addOperationParameters', dict(operationId=operation_id, params=_list(operation_parameters)))

    def remove_operation_parameters(self, operation_id, parameter_names=None):
        return self._request('removeOperationParameters', dict(operationId=operation_id, params=_list(parameter_names)))

    def get_operation_resources(self, operation_id):
        return self._request('getOperationResources', dict(operationId=operation_id))

    def add_operation_resources(self, operation_id, operation_resources=None):
        return self._request('addOperationResources', dict(operationId=operation_id, resources=_list(operation_resources)))

    def remove_operation_resources(self, operation_id, resource_names=None):
        return self._request('removeOperationResources', dict(operationId=operation_id, resourceNames=_list(resource_names)))

    def composite_update_operation_blocks(self, operation_id, blocks=None, block_update_strategy='minimum_not_deprecated', dry_run=False):
        return self._request(
            'compositeUpdateOperationBlocks',
            dict(
                operationId=operation_id,
                blocks=_list(blocks),
                blockUpdateStrategy=block_update_strategy,
                dryRun=dry_run
            )
        )

    def layout_workflow_instance(self, workflow_instance_id):
        return self._request('layoutWorkflowInstance', dict(workflowInstanceId=workflow_instance_id))

    def find_projects_by_participants(self, users):
        if isinstance(users, str):
            users = [users]

        return self._request(
            'findProjectsByParticipants',
            dict(
                userIds=users
            )
        )

    def add_participants_to_projects(self, users, projects):
        return self._request(
            'addParticipantsToProjects',
            dict(
                projectCodes=projects,
                userIds=users
            )
        )


NIRVANA_DEBUG_ALL = 5

NIRVANA_DATETIME_PATTERN = '%Y-%m-%dT00:00:00+0300'


def _list(data):
    return data if data is None or isinstance(data, list) else [data]


def _permissions(permissions):
    return permissions if permissions is None else dict(permissions=_list(permissions))
