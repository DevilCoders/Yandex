import time
import socket
import os.path
import graphviz as gv

import logging
from hashlib import md5
import tempfile
from collections import OrderedDict

from .workflow import Workflow, WorkflowInstance
from .execution_state import ExecutionStatus, ExecutionResult
from .parameter_classes import (BlockPattern, ComputableParameterValue, ParameterValue, ParameterReference, GlobalParameter, DataBlock,
                                BlockPosition,
                                OperationBlock, ScpUploadParameters, HttpUploadParameters, SyncHttpUploadParameters, UploadMethods, Expressions)
from .blocks.base_block import BlockType, fix_attr_name, BaseBlock, BaseDataBlock
from .blocks.processor_parameters import ProcessorParameters

logger = logging.getLogger(__name__)


def create_workflow(nirvana, workflow_name, roots,
                    quota_project_id=None, follow_outputs=True, project_code=None,
                    description=None, ns_id=None):
    wf_object = nirvana.create_workflow(workflow_name, quota_project_id, project_code,
                                        description=description, ns_id=ns_id)
    workflow = Workflow(nirvana, wf_object)
    logger.info('Flow link: {}'.format(workflow.get_url()))

    if roots is None:
        roots = []
    elif isinstance(roots, BaseBlock):
        roots = [roots]

    _fill_workflow(workflow, roots, follow_outputs)
    logger.info('Flow was created')
    return workflow


def create_workflow_instance(nirvana, workflow_id, roots, follow_outputs=True, quota_project_id=None):
    instance_info = {
        'meta': {
            'quotaProjectId': quota_project_id
        }
    }
    wf_instance_id = nirvana.create_workflow_instance(workflow_id, instance_info)
    workflow = Workflow(nirvana, workflow_id)
    workflow_instance = WorkflowInstance(nirvana, wf_instance_id)

    if roots is None:
        roots = []
    elif isinstance(roots, BaseBlock):
        roots = [roots]

    _fill_workflow(workflow, roots, follow_outputs)
    logger.info('Instance was created')
    return workflow_instance


def update_workflow(workflow, roots, follow_outputs=True):
    workflow.remove_all()
    workflow.parameters.clear()
    logger.info('Flow link: {}'.format(workflow.get_url()))

    if roots is None:
        roots = []
    if isinstance(roots, BaseBlock):
        roots = [roots]

    _fill_workflow(workflow, roots, follow_outputs)
    logger.info('Flow was created')
    return workflow


def add_nodes_to_workflow(workflow, nodes):
    if nodes is None:
        nodes = []
    elif isinstance(nodes, BaseBlock):
        nodes = [nodes]

    _add_nodes_to_workflow(workflow, nodes, follow_outputs=True)
    logger.info('Added nodes to workflow')
    return workflow


def create_workflow_parameters(workflow):
    workflow.parameters.construct()
    logger.info('Workflow parameters were created')
    return workflow


def wait_workflow(workflow, sleep_time=30, max_error=10):
    state = None
    progress = -1
    error_count = 0
    while not state or state.status != ExecutionStatus.completed:
        try:
            state = workflow.get_execution_state()
            error_count = 0
        except Exception as e:
            error_count += 1
            logger.exception(e)
            if error_count > max_error:
                raise e

        if state and state.progress != progress:
            progress = state.progress
            logger.info('Running flow: status {0}, progress {1}, message {2}'.format(state.status, state.progress,
                                                                                     getattr(state, 'message', 'None')))
        time.sleep(sleep_time)

    if state.result != ExecutionResult.success:
        raise NirvanaWorkflowExecutionError(workflow.id, state)

    logger.info('End flow: {0}'.format(state))


def run_workflow(workflow, sleep_time=30, max_error=10):
    logger.info('Validate flow: {0}'.format(workflow.validate()))
    logger.info('Start flow: {0}'.format(workflow.start()))
    wait_workflow(workflow, sleep_time, max_error)


# strict_reuse - rerun operations if yt tables changed
def run_graph(
    nirvana, workflow_name, roots, quota_project_id=None, follow_outputs=True, project_code=None,
    description=None, ttl=None, threads=None, strict_reuse=False, ns_id=None
):
    workflow = create_workflow(
        nirvana, workflow_name, roots,
        quota_project_id=quota_project_id,
        follow_outputs=follow_outputs,
        project_code=project_code,
        description=description,
        ns_id=ns_id
    )
    if ttl is not None:
        nirvana.edit_workflow(
            workflow.id, execution_params={'resultsTtlDays': ttl}
        )
    if threads is not None:
        nirvana.edit_workflow(
            workflow.id, execution_params={'concurrentOperationsCount': threads}
        )
    if strict_reuse:
        nirvana.edit_workflow(
            workflow.id, execution_params={'cachedExternalDataReusePolicy': "reuse_if_not_modified_strict"}
        )

    workflow.validate()
    workflow.start()
    wait_workflow(workflow)

    return workflow


def cached_upload_data(nirvana, name, *args, **kws):
    results = nirvana.find_data(name)
    if results:
        return results[0]['dataId']
    return upload_data(nirvana, name, *args, **kws)


def create_data_from_string(nirvana_api, content, name_fmt='{}', data_type='text', ttl_days=14):
    name = name_fmt.format(md5(content).hexdigest())
    with tempfile.NamedTemporaryFile() as f:
        f.write(content)
        f.flush()
        data_id = cached_upload_data(
            nirvana_api, name=name, file_path=f.name,
            data_type=data_type, method=UploadMethods.SYNC_HTTP, ttl_days=ttl_days)
    data_file = BaseDataBlock(guid=data_id, name=name).outputs.data
    return data_file


def upload_data(nirvana, name, file_path, data_type, archive=None, sleep_time=10, max_error=10, method=None, upload_parameters=None, quota_project=None, ttl_days=None, tags=None, description=None, file_name='content'):  # noqa

    data_id = nirvana.create_data(name, data_type, quota_project=quota_project, ttl_days=ttl_days, description=description)
    if tags:
        nirvana.edit_data_tags(data_id, {"names": tags, "strictTagCheck": False})

    logger.info('Data block id: {0}'.format(data_id))

    is_archive = _is_archive(file_path) if archive is None else archive

    if method is None:
        if file_path.startswith('http://') or file_path.startswith('https://'):
            method = UploadMethods.HTTP
        else:
            method = UploadMethods.SCP

    if method == UploadMethods.SYNC_HTTP:
        upload_parameters = SyncHttpUploadParameters(archive=is_archive)
        nirvana.upload_data_multipart(data_id, upload_parameters, open(file_path, 'rb'), file_name)
    else:
        if method == UploadMethods.SCP:
            scp_path = file_path
            if ':' not in scp_path:
                scp_path = '{0}:{1}'.format(socket.getfqdn(), os.path.abspath(scp_path))
            upload_parameters = ScpUploadParameters(remote_path=scp_path, archive=is_archive)
        elif method == UploadMethods.HTTP:
            upload_parameters = HttpUploadParameters(url=file_path, archive=is_archive)
        elif method == UploadMethods.SANDBOX:
            assert upload_parameters is not None, 'specify parameters if using sandbox upload'
        else:
            assert False, 'unreachable'
        nirvana.upload_data(data_id, upload_parameters)

    state = None
    progress = -1
    error_count = 0
    while True:
        try:
            state = nirvana.get_data_upload_state(data_id)
        except Exception as e:
            error_count += 1
            logger.exception(e)
            if error_count > max_error:
                raise e

        if state and state.status == ExecutionStatus.completed:
            break

        if state and state.progress != progress:
            progress = state.progress
            logger.info('Uploading data: {0}'.format(state))

        time.sleep(sleep_time)

    if state.result != ExecutionResult.success:
        raise NirvanaExecutionError(data_id, state)

    logger.info('Upload end: {0}'.format(state))
    return data_id


class NirvanaExecutionError(Exception):

    def __init__(self, guid, state):
        self.id = guid
        self.state = state

    def __str__(self):
        if not self.state:
            return 'ExecutionState: id {0}, state None'.format(self.id)
        return 'ExecutionState: id {0}, status {1}, result {2}\n{3}'.format(
            self.id, self.state.status, self.state.result, self.state)


class NirvanaWorkflowExecutionError(NirvanaExecutionError):
    pass


def _fill_workflow(workflow, roots, follow_outputs):  # noqa
    if len(roots) == 0:
        return

    nodes = _traverse(roots, follow_outputs)
    _add_nodes_to_workflow(workflow, nodes, follow_outputs)


def _add_nodes_to_workflow(workflow, nodes, follow_outputs=True):
    if not len(nodes):
        return

    data_nodes, operation_nodes = _split_nodes(nodes)
    workflow.add_data_blocks([DataBlock(node.guid, node.name, node.code) for node in data_nodes])
    workflow.add_operation_blocks([OperationBlock(node.guid, node.name, node.code) for node in operation_nodes])

    global_params = _get_global_parameters(operation_nodes)
    workflow.parameters.declare(global_params)

    block_params = []
    references = []
    for node in operation_nodes:
        params, _ = _get_parameters(node)
        values, refs = _split_parameters(params)
        if values:
            block_params.append((BlockPattern(code=node.code), values))
        if refs:
            references.append((node, refs))
    with workflow.nirvana.batch():
        for param in block_params:
            workflow.set_block_parameters(*param)
    with workflow.nirvana.batch():
        for ref in references:
            workflow.parameters.set_references(*ref)

    data_connections, operation_connections, execution_connections = _get_connections(nodes, follow_outputs)
    with workflow.nirvana.batch():
        for connection in data_connections:
            workflow.connect_data_blocks(*connection)
    with workflow.nirvana.batch():
        for connection in operation_connections:
            workflow.connect_operation_blocks(*connection)
    with workflow.nirvana.batch():
        for connection in execution_connections:
            workflow.connect_operation_blocks_by_execution_order(*connection)


def _get_connections(nodes, follow_outputs=True):
    def make_data_connection(output_conn, input_conn):
        return (
            BlockPattern(code=output_conn.obj.code),
            BlockPattern(code=input_conn.obj.code),
            input_conn.name,
        )

    def make_operation_connection(output_conn, input_conn):
        return (
            BlockPattern(code=output_conn.obj.code),
            output_conn.name,
            BlockPattern(code=input_conn.obj.code),
            input_conn.name,
        )

    def make_execution_connection(output_conn, input_conn):
        return (
            BlockPattern(code=output_conn.obj.code),
            BlockPattern(code=input_conn.obj.code),
        )

    def update_data_and_operation_connections(output_conn, input_conn):
        if output_conn.obj._type == BlockType.data:
            data_connections[make_data_connection(output_conn, input_conn)] = True
        elif output_conn.obj._type == BlockType.operation:
            operation_connections[make_operation_connection(output_conn, input_conn)] = True

    data_connections = OrderedDict()
    operation_connections = OrderedDict()
    execution_connections = OrderedDict()
    for node in nodes:
        for _, input_connection in node.get_ordered_inputs():
            for output_connection in input_connection.get_ordered_links():
                update_data_and_operation_connections(output_connection, input_connection)

        execute_after = node.get_execute_after()
        for execute_before in execute_after.get_ordered_links():
            execution_connections[make_execution_connection(execute_before, execute_after)] = True

        if follow_outputs:
            for _, output_connection in node.get_ordered_outputs():
                for input_connection in output_connection.get_ordered_links():
                    update_data_and_operation_connections(output_connection, input_connection)

            execute_before = node.get_execute_before()
            for execute_after in execute_before.get_ordered_links():
                execution_connections[make_execution_connection(execute_before, execute_after)] = True
    return data_connections.keys(), operation_connections.keys(), execution_connections.keys()


def _get_parameters(block):
    params = []
    global_params = []
    visited_global_params = set()

    for name in block.parameters + ProcessorParameters[block.processor_type]:
        value = getattr(block, fix_attr_name(name))
        if value is None:
            continue

        if isinstance(value, GlobalParameter):
            params.append(ParameterReference(name, value.parameter))

            if id(value) not in visited_global_params:
                global_params.append(value)
                visited_global_params.add(id(value))

        elif isinstance(value, Expressions):
            params.append(ComputableParameterValue(name, expressions=value))

        else:
            params.append(ParameterValue(name, value))

    return params, global_params


def _split_parameters(params):
    values = []
    references = []

    for param in params:
        if isinstance(param, ParameterReference):
            references.append(param)
        else:
            assert isinstance(param, (ParameterValue, ComputableParameterValue))
            values.append(param)

    return values, references


def _get_global_parameters(blocks):
    global_params = []
    visited_global_params = set()

    for block in blocks:
        _, block_global_params = _get_parameters(block)

        for param in block_global_params:
            assert isinstance(param, GlobalParameter)

            if id(param) not in visited_global_params:
                global_params.append(param)
                visited_global_params.add(id(param))

    return global_params


def _traverse(roots, follow_outputs):
    blocks = set(roots)
    stack = list(roots)

    def add_to_stack(connection):
        for link in connection.get_ordered_links():
            if link.obj not in blocks:
                blocks.add(link.obj)
                stack.append(link.obj)

    while stack:
        node = stack.pop()

        for (_, value) in node.get_ordered_inputs():
            add_to_stack(value)

        add_to_stack(node.get_execute_after())

        if follow_outputs:
            add_to_stack(node.get_execute_before())
            for (_, value) in node.get_ordered_outputs():
                add_to_stack(value)

    return sorted(blocks, key=lambda x: x.counter_id)


def _split_nodes(nodes):
    data_nodes = [node for node in nodes if node._type == BlockType.data]
    operation_nodes = [node for node in nodes if node._type == BlockType.operation]
    return data_nodes, operation_nodes


def _is_archive(filename):
    _, ext = os.path.splitext(filename)
    return ext == '.gz' or ext == '.bz'


def layout_workflow(workflow, roots, follow_outputs, workflow_instance_id=None):
    blocks = _traverse(roots, follow_outputs)
    graph = gv.Digraph(format='plain',
                       edge_attr={'headport': 'n', 'tailport': 's'},
                       node_attr={'shape': 'box', 'height': '1.01', 'width': '1.76'},
                       graph_attr={'splines': 'false', 'ranksep': '0.75'})
    for b in blocks:
        inputs = b.get_ordered_inputs()
        ports = ['n'] * len(inputs)
        if len(ports) > 1:
            ports[0] = 'nw'
            ports[-1] = 'ne'
        for (_, connection), port in zip(inputs, ports):
            for link in connection.get_ordered_links():
                graph.edge('b'+link.obj.code, 'b'+b.code, headport=port)
    positions = {}
    for line in graph.pipe().split('\n'):
        if line.startswith('node '):
            _, label, x, y, _ = line.split(' ', 4)
        elif line.startswith('graph'):
            label, x, y, _ = line.split(' ', 3)
        else:
            continue
        label = label.strip('"')
        positions[label[1:]] = (float(x), float(y))
    max_y = max(p[1] for p in positions.itervalues())
    with workflow.nirvana.batch():
        for b in blocks:
            pos = positions.get(b.code)
            if pos:
                x, y = pos
                y = max_y - y
                workflow.set_block_position(BlockPattern(code=b.code), BlockPosition(100*x, 100*y), workflow_instance_id)
