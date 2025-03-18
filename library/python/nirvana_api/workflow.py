from collections import OrderedDict

from .parameter_classes import BlockPattern, ComputableParameterValue, Expressions, GlobalParameter, ParameterValue
from .blocks.base_block import fix_attr_name


class WorkflowParameterRefs:
    def __init__(self, node, refs):
        self.node = node
        self.refs = refs


class WorkflowParameters:
    def __init__(self, workflow):
        self._parameters = OrderedDict()
        self._references = OrderedDict()
        self._workflow = workflow

    def set(self, **kwarg):
        for name in kwarg:
            setattr(self, name, kwarg[name])

    def declare(self, params):
        for global_param in params:
            assert isinstance(global_param, GlobalParameter)
            attr_name = fix_attr_name(global_param.parameter)
            if not hasattr(self, attr_name):
                setattr(self, attr_name, global_param.value)

            self._parameters[attr_name] = global_param

    def set_references(self, node, refs):
        self._references[node] = WorkflowParameterRefs(node, refs)

        for ref in refs:
            attr_name = fix_attr_name(ref.derivedFrom)
            assert hasattr(self, attr_name)

    def clear(self):
        self._parameters.clear()
        self._references.clear()

    def construct(self):
        global_params = self._parameters.values()
        self._workflow.add_global_parameters_if_not_exist(global_params)

        param_values = []
        for name in self._parameters:
            global_parameter = self._parameters[name]
            value = getattr(self, name)
            if value is not None:
                if isinstance(value, Expressions):
                    param_values.append(
                        ComputableParameterValue(
                            global_parameter.parameter,
                            expressions=value
                        )
                    )
                else:
                    param_values.append(ParameterValue(global_parameter.parameter, value))

        if param_values:
            self._workflow.set_global_parameters(param_values)

        for refs_in_node in self._references.values():
            node = refs_in_node.node
            refs = refs_in_node.refs

            self._workflow.link_block_parameters(BlockPattern(code=node.code), refs, strict_parameter_matching=True)


class Workflow:
    def __init__(self, nirvana, workflow_id):
        self.id = workflow_id
        self.nirvana = nirvana
        self.parameters = WorkflowParameters(self)

    def get(self):
        return self.nirvana.get_workflow(self.id)

    def get_url(self):
        return self.nirvana.get_workflow_url(self.id)

    def edit(self, name=None, owner=None, description=None, quota_project_id=None, execution_params=None,
             permissions=None, tags=None):
        return self.nirvana.edit_workflow(self.id, name, owner, description, quota_project_id, execution_params,
                                          permissions, tags)

    def validate(self):
        return self.nirvana.validate_workflow(self.id)

    def start(self):
        return self.nirvana.start_workflow(self.id)

    def stop(self):
        return self.nirvana.stop_workflow(self.id)

    def subscribe_user(self, user, states=None):
        return self.nirvana.subscribe_to_workflow(self.id, user, states)

    def unsubscribe_user(self, user):
        return self.nirvana.unsubscribe_from_workflow(self.id, user)

    def get_subscriptions(self):
        return self.nirvana.get_subscriptions(self.id)

    def get_meta_data(self):
        return self.nirvana.get_workflow_meta_data(self.id)

    def get_summary(self, block_patterns=None):
        return self.nirvana.get_workflow_summary(workflow_id=self.id, block_patterns=block_patterns)

    def get_execution_state(self, block_patterns=None, workflow_instance_id=None):
        return self.nirvana.get_execution_state(workflow_id=self.id, workflow_instance_id=workflow_instance_id,
                                                block_patterns=block_patterns)

    def get_block_meta_data(self, block_patterns=None, params=None, workflow_instance_id=None):
        return self.nirvana.get_block_meta_data(
            workflow_id=self.id,
            workflow_instance_id=workflow_instance_id,
            block_patterns=_list(block_patterns),
            params=_list(params)
        )

    def get_block_parameters(self, block_patterns=None, params=None, workflow_instance_id=None, return_nulls=False):
        return self.nirvana.get_block_parameters(
            workflow_id=self.id,
            workflow_instance_id=workflow_instance_id,
            block_patterns=_list(block_patterns),
            params=_list(params),
            return_nulls=return_nulls,
        )

    def get_block_results(self, block_patterns=None, outputs=None, workflow_instance_id=None):
        return self.nirvana.get_block_results(
            workflow_id=self.id,
            workflow_instance_id=workflow_instance_id,
            block_patterns=_list(block_patterns),
            outputs=_list(outputs)
        )

    def set_block_permissions(self, permissions, blocks=None, workflow_instance_id=None):
        return self.nirvana._request(
            'setBlockPermissions',
            dict(workflowId=self.id, workflowInstanceId=workflow_instance_id, blocks=_list(blocks),
                 permissions=dict(permissions=_list(permissions)))
        )

    def add_data_blocks(self, blocks, workflow_instance_id=None):
        return self.nirvana._request('addDataBlocks', dict(workflowId=self.id, workflowInstanceId=workflow_instance_id,
                                                           blocks=_list(blocks)))

    def add_operation_blocks(self, blocks, workflow_instance_id=None):
        return self.nirvana._request('addOperationBlocks',
                                     dict(workflowId=self.id, workflowInstanceId=workflow_instance_id,
                                          blocks=_list(blocks)))

    def edit_data_blocks(self, block_patterns, edit_block, workflow_instance_id=None):
        return self.nirvana._request(
            'editDataBlocks',
            dict(workflowId=self.id, workflowInstanceId=workflow_instance_id, blocks=_list(block_patterns),
                 dataBlockParams=edit_block)
        )

    def edit_operation_blocks(self, block_patterns, edit_block, workflow_instance_id=None):
        return self.nirvana._request(
            'editOperationBlocks',
            dict(workflowId=self.id, workflowInstanceId=workflow_instance_id, blocks=_list(block_patterns),
                 operationBlockParams=edit_block)
        )

    def get_block_position(self, block_patterns, workflow_instance_id=None):
        return self.nirvana._request(
            'getBlockPosition',
            dict(workflowId=self.id, workflowInstanceId=workflow_instance_id, blocks=_list(block_patterns))
        )

    def set_block_position(self, block_pattern, block_position, workflow_instance_id=None):
        return self.nirvana._request(
            'setBlockPosition',
            dict(workflowId=self.id, workflowInstanceId=workflow_instance_id, block=block_pattern,
                 params=block_position)
        )

    def update_operation_blocks(self, block_patterns=None, workflow_instance_id=None):
        return self.nirvana._request('updateOperationBlocks',
                                     dict(workflowId=self.id, workflowInstanceId=workflow_instance_id,
                                          blocks=_list(block_patterns)))

    def set_block_parameters(self, block_patterns=None, param_values=None, workflow_instance_id=None):
        return self.nirvana._request(
            'setBlockParameters',
            dict(workflowId=self.id, workflowInstanceId=workflow_instance_id, blocks=_list(block_patterns),
                 params=_list(param_values))
        )

    def connect_data_blocks(self, source_block_patterns, dest_block_patterns, dest_inputs,
                            connection_name_template=None, workflow_instance_id=None):
        return self.nirvana._request(
            'connectDataBlocks',
            dict(
                workflowId=self.id, workflowInstanceId=workflow_instance_id,
                sourceBlocks=_list(source_block_patterns),
                destBlocks=_list(dest_block_patterns), destInputs=_list(dest_inputs),
                connectionNameTemplate=connection_name_template
            )
        )

    def connect_operation_blocks(self, source_block_patterns, source_outputs, dest_block_patterns,
                                 dest_inputs, connection_name_template=None, workflow_instance_id=None):
        return self.nirvana._request(
            'connectOperationBlocks',
            dict(
                workflowId=self.id, workflowInstanceId=workflow_instance_id,
                sourceBlocks=_list(source_block_patterns), sourceOutputs=_list(source_outputs),
                destBlocks=_list(dest_block_patterns), destInputs=_list(dest_inputs),
                connectionNameTemplate=connection_name_template
            )
        )

    def connect_operation_blocks_by_execution_order(self, source_block_patterns, dest_block_patterns,
                                                    workflow_instance_id=None):
        return self.nirvana._request(
            'connectOperationBlocksByExecutionOrder',
            dict(
                workflowId=self.id, workflowInstanceId=workflow_instance_id,
                sourceBlocks=_list(source_block_patterns),
                destBlocks=_list(dest_block_patterns)
            )
        )

    def disconnect_blocks(self, block_connections, workflow_instance_id=None):
        return self.nirvana._request(
            'disconnectBlocks',
            dict(workflowId=self.id, workflowInstanceId=workflow_instance_id, blockConnections=_list(block_connections))
        )

    def create_dynamic_connection(self, source_block_patterns, source_outputs, dest_block_patterns,
                                  workflow_instance_id=None):
        return self.nirvana._request(
            'createDynamicConnection',
            dict(
                workflowId=self.id, workflowInstanceId=workflow_instance_id,
                sourceBlocks=_list(source_block_patterns), sourceOutputs=_list(source_outputs),
                destBlocks=_list(dest_block_patterns)
            )
        )

    def remove_blocks(self, block_patterns=None):
        return self.nirvana._request('removeBlocks', dict(workflowId=self.id, blocks=_list(block_patterns)))

    def remove_all(self):
        description = self.get()
        self.disconnect_blocks([c.guid for c in description.connections])
        self.remove_blocks([to_block_pattern(b) for b in description.blocks])

    def add_global_parameters(self, global_params=None):
        return self.nirvana.add_global_parameters(workflow_id=self.id, global_params=_list(global_params))

    def add_global_parameters_if_not_exist(self, global_params=None):
        flow_params = self.get_global_parameters()
        existing_params = {p.parameter for p in flow_params} if isinstance(flow_params, list) else set()
        return self.add_global_parameters([p for p in (global_params or []) if p.parameter not in existing_params])

    def get_global_parameters(self):
        return self.nirvana.get_global_parameters(workflow_id=self.id)

    def get_global_parameters_meta_data(self):
        return self.nirvana.get_global_parameters_meta_data(workflow_id=self.id)

    def set_global_parameters(self, param_values=None):
        return self.nirvana.set_global_parameters(workflow_id=self.id, param_values=param_values)

    def link_block_parameters(self, block_patterns=None, param_refs=None, strict_parameter_matching=None,
                              workflow_instance_id=None):
        return self.nirvana._request(
            'linkBlockParameters',
            dict(
                workflowId=self.id, workflowInstanceId=workflow_instance_id,
                blocks=_list(block_patterns), params=_list(param_refs),
                strictParameterMatching=strict_parameter_matching
            )
        )

    def unlink_block_parameters(self, block_patterns=None, params=None, strict_parameter_matching=None,
                                workflow_instance_id=None):
        return self.nirvana._request(
            'unlinkBlockParameters',
            dict(
                workflowId=self.id, workflowInstanceId=workflow_instance_id,
                blocks=_list(block_patterns), params=_list(params),
                strictParameterMatching=strict_parameter_matching
            )
        )

    def get_instance(self):
        meta_data = self.get_meta_data()
        instance_id = meta_data['instanceId']
        return WorkflowInstance(
            nirvana=self.nirvana,
            instance_id=instance_id
        )


class WorkflowInstance:
    def __init__(self, nirvana, instance_id):
        self.id = instance_id
        self.nirvana = nirvana

    def get(self):
        return self.nirvana.get_workflow(workflow_instance_id=self.id)

    def validate(self):
        return self.nirvana.validate_workflow(workflow_instance_id=self.id)

    def start(self):
        return self.nirvana.start_workflow(workflow_instance_id=self.id)

    def stop(self):
        return self.nirvana.stop_workflow(workflow_instance_id=self.id)

    def get_meta_data(self):
        return self.nirvana.get_workflow_meta_data(workflow_instance_id=self.id)

    def get_execution_state(self, block_patterns=None):
        return self.nirvana.get_execution_state(workflow_instance_id=self.id, block_patterns=block_patterns)

    def add_global_parameters(self, global_params=None):
        return self.nirvana.add_global_parameters(workflow_instance_id=self.id, global_params=_list(global_params))

    def get_global_parameters(self):
        return self.nirvana.get_global_parameters(workflow_instance_id=self.id)

    def get_global_parameters_meta_data(self):
        return self.nirvana.get_global_parameters_meta_data(workflow_instance_id=self.id)

    def set_global_parameters(self, param_values=None):
        return self.nirvana.set_global_parameters(workflow_instance_id=self.id, param_values=param_values)

    def get_block_results(self, block_patterns=None, outputs=None):
        return self.nirvana.get_block_results(
            workflow_instance_id=self.id,
            block_patterns=_list(block_patterns),
            outputs=_list(outputs)
        )

    def edit(self, name=None, owner=None, description=None, quota_project_id=None, execution_params=None,
             permissions=None, tags=None):
        return self.nirvana.edit_workflow(
            workflow_instance_id=self.id,
            name=name,
            owner=owner,
            description=description,
            quota_project_id=quota_project_id,
            execution_params=execution_params,
            permissions=permissions,
            tags=tags
        )

    def edit_data_blocks(self, block_patterns, edit_block):
        return self.nirvana._request(
            'editDataBlocks',
            dict(workflowInstanceId=self.id, blocks=_list(block_patterns), dataBlockParams=edit_block)
        )

    def layout(self):
        return self.nirvana.layout_workflow_instance(
            workflow_instance_id=self.id
        )


def to_block_pattern(b):
    return BlockPattern(code=b.blockCode)


def _list(data):
    return data if data is None or isinstance(data, list) else [data]
