# coding=utf-8
from .base import TestBase, FirstBlock, SecondBlock, ThirdBlock, FourthBlock, FifthBlock, SingletonFirstBlock, fix_workflow_description

import json

import nirvana_api.json_rpc as json_rpc

from nirvana_api.parameter_classes import GlobalParameter, ParameterType
from nirvana_api.highlevel_api import update_workflow, create_workflow_parameters


class TestHighlevelApi(TestBase):
    def test_empty_workflow(self):
        update_workflow(self.workflow, None)
        description = json.dumps(self.workflow.get(), cls=json_rpc.DefaultEncoder)
        assert description == '{"connections": [], "blocks": []}'

    def test_set_inputs_from_ctor_canonical(self):
        return self.canonical_desc

    def test_set_inputs_from_attr(self):
        first = FirstBlock(code='1')
        second = SecondBlock(code='2')
        third = ThirdBlock(code='3')

        third.inputs.tsv = second.outputs.tsv
        second.inputs.json = first.outputs.json

        update_workflow(self.workflow, third)
        description = json.dumps(fix_workflow_description(self.workflow.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)
        assert self.canonical_desc == description

    def test_set_outputs_from_attr(self):
        first = FirstBlock(code='1')
        second = SecondBlock(code='2')
        third = ThirdBlock(code='3')

        first.outputs.json = second.inputs.json
        second.outputs.tsv = third.inputs.tsv

        update_workflow(self.workflow, third)
        description = json.dumps(fix_workflow_description(self.workflow.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)
        assert self.canonical_desc == description

    def test_different_root(self):
        first = FirstBlock(code='1')
        second = SecondBlock(code='2')
        third = ThirdBlock(code='3')

        third.inputs.tsv = second.outputs.tsv
        second.inputs.json = first.outputs.json

        update_workflow(self.workflow, first)
        description = json.dumps(fix_workflow_description(self.workflow.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)
        assert self.canonical_desc == description

        update_workflow(self.workflow, second)
        description = json.dumps(fix_workflow_description(self.workflow.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)
        assert self.canonical_desc == description

        update_workflow(self.workflow, third)
        description = json.dumps(fix_workflow_description(self.workflow.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)
        assert self.canonical_desc == description

    def test_set_few_inputs_outputs(self):
        first_1 = FirstBlock(code='1.1')
        first_2 = FirstBlock(code='1.2')
        first_3 = FirstBlock(code='1.3')
        second = SecondBlock(code='2')
        third = ThirdBlock(code='3')

        first_1.outputs.json = second.inputs.json
        first_2.outputs.json = second.inputs.json
        first_1.outputs.binary = [second.inputs.binary, third.inputs.binary]
        first_2.outputs.binary = [second.inputs.binary, third.inputs.binary]

        second.outputs.tsv = third.inputs.tsv

        third.inputs.json = [first_1.outputs.json, first_2.outputs.json]
        third.inputs.json.connect(first_3.outputs.json)

        update_workflow(self.workflow, third)
        canonical_desc = json.dumps(fix_workflow_description(self.workflow.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)

        update_workflow(self.workflow, first_1)
        description = json.dumps(fix_workflow_description(self.workflow.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)
        assert canonical_desc == description

        update_workflow(self.workflow, first_2)
        description = json.dumps(fix_workflow_description(self.workflow.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)
        assert canonical_desc == description

        update_workflow(self.workflow, second)
        description = json.dumps(fix_workflow_description(self.workflow.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)
        assert canonical_desc == description

        return canonical_desc

    def test_one_name_for_everything(self):
        first = FirstBlock(code='1')
        second = SecondBlock(code='2')
        third = ThirdBlock(code='3')
        fourth = FourthBlock(code='4', one_name='param')

        first.outputs.json = second.inputs.json
        first.outputs.binary = [second.inputs.binary, third.inputs.binary]
        second.outputs.tsv = third.inputs.tsv
        third.inputs.json = [first.outputs.json, fourth.outputs.one_name]
        fourth.inputs.one_name = first.outputs.json

        update_workflow(self.workflow, third)
        canonical_desc = json.dumps(fix_workflow_description(self.workflow.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)

        first = FirstBlock(code='1')
        second = SecondBlock(code='2', json=first.outputs.json, binary=first.outputs.binary)
        fourth = FourthBlock(code='4', one_name=first.outputs.json)
        fourth.one_name = 'param'
        third = ThirdBlock(code='3', json=[first.outputs.json, fourth.outputs.one_name], tsv=second.outputs.tsv, binary=first.outputs.binary)

        update_workflow(self.workflow, third)
        description = json.dumps(fix_workflow_description(self.workflow.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)
        assert canonical_desc == description

        first = FirstBlock(code='1')
        second = SecondBlock(code='2', json=first.outputs.json, binary=first.outputs.binary)
        fourth = FourthBlock(code='4')
        fourth.one_name = 'param'
        fourth.inputs.one_name = first.outputs.json
        third = ThirdBlock(code='3', json=[first.outputs.json, fourth.outputs.one_name], tsv=second.outputs.tsv, binary=first.outputs.binary)

        update_workflow(self.workflow, third)
        description = json.dumps(fix_workflow_description(self.workflow.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)
        assert canonical_desc == description

        return canonical_desc

    def test_set_local_name_guid(self):
        first_1 = FirstBlock(code='1.1', name='__first__')
        first_2 = FirstBlock(code='1.2')
        first_3 = FirstBlock(code='1.3', name='__s_f__')
        third_1 = ThirdBlock(code='3.1', name='third', guid=SecondBlock.guid)
        third_2 = ThirdBlock(code='3.2', name='real_third')

        first_1.outputs.binary = [third_1.inputs.binary, third_2.inputs.binary]
        first_2.outputs.binary = [third_1.inputs.binary, third_2.inputs.binary]

        third_1.inputs.json = [first_1.outputs.json, first_2.outputs.json]
        third_1.inputs.json.connect(first_3.outputs.json)
        third_2.inputs.json = [first_1.outputs.json, first_2.outputs.json]

        update_workflow(self.workflow, third_2)
        canonical_desc = json.dumps(fix_workflow_description(self.workflow.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)
        return canonical_desc

    def test_global_params(self):
        param_int = GlobalParameter('int_param', ParameterType.integer)
        param_str = GlobalParameter('str_param', ParameterType.string)
        param_bool = GlobalParameter('bool_param', ParameterType.boolean)
        # FIXME: add test for enum, when API is fixed, https://st.yandex-team.ru/NIRVANA-4946

        first_1 = FirstBlock(code='1.1', name='first_1', test_int=param_int)
        first_2 = FirstBlock(code='1.2', name='first_2', test_int=param_int, test_str=param_str)
        first_3 = FirstBlock(code='1.3', name='first_3', test_bool=param_bool)

        third = ThirdBlock(code='3', name='third')
        third.inputs.json = [first_1.outputs.json, first_2.outputs.json, first_3.outputs.json]

        update_workflow(self.workflow, third)
        self.workflow.parameters.set(str_param='xyz', bool_param=True)
        create_workflow_parameters(self.workflow)  # this call makes changes to workflow that can't be cleaned up (yet, https://st.yandex-team.ru/NIRVANA-4944)

        descr = _fix_global_parameters_description(self.workflow.get_global_parameters())
        return json.dumps(descr, sort_keys=True, cls=json_rpc.DefaultEncoder)

    def test_singleton_blocks(self):
        first_1 = SingletonFirstBlock(code='1.1', test_int=1)
        first_2 = SingletonFirstBlock(code='1.2', test_int=1)
        first_3 = SingletonFirstBlock(code='1.3', test_int=2)
        second_1 = SecondBlock(code='2.1', json=first_1.outputs.json, binary=first_2.outputs.binary)
        second_2 = SecondBlock(code='2.2', json=first_3.outputs.json)
        update_workflow(self.workflow, [second_1, second_2])
        return json.dumps(fix_workflow_description(self.workflow.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)

    def test_aliases_and_strict_naming(self):
        first = FirstBlock(code='1', anyName='foo', is_strict=False)  # is_strict=False, no error
        fifth_1 = FifthBlock(code='5.1', OneName='value')
        fifth_2 = FifthBlock(code='5.2', oneName='value')

        error_1 = None
        try:
            FifthBlock(code='5.3', OneName='value1', oneName='value2')
        except AssertionError as e:
            error_1 = e

        assert error_1

        error_2 = None
        try:
            FifthBlock(code='5.4', otherName='value')
        except AssertionError as e:
            error_2 = e

        assert error_2

        fifth_5 = FifthBlock(code='5.5')
        fifth_5.configure_parameters(OneName='value')
        fifth_5.configure_inputs(OneName=first.outputs.json)

        fifth_6 = FifthBlock(code='5.6')
        fifth_6.inputs.OneName.connect(fifth_5.outputs.oneName)

        update_workflow(self.workflow, [fifth_1, fifth_2, fifth_5, fifth_6])
        canonical_desc = json.dumps(fix_workflow_description(self.workflow.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)

        return canonical_desc


def _fix_global_parameters_description(description):
    sorted_description = list(description)
    sorted_description.sort(key=lambda param: param.parameter)
    return sorted_description
