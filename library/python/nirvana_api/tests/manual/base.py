# coding=utf-8

import json

import logging
logger = logging.getLogger(__name__)

import requests

import nirvana_api.json_rpc as json_rpc

from nirvana_api.api import NirvanaApi
from nirvana_api.blocks import BaseBlock, singleton_block
from nirvana_api.highlevel_api import create_workflow

__all__ = [
    "TestBase",
    "FirstBlock",
    "SecondBlock",
    "ThirdBlock",
    "FourthBlock",
    "FifthBlock",
    "SingletonFirstBlock",
    "fix_workflow_description"
]


def fix_workflow_description(description):
    for c in description.connections:
        c.guid = ''
        c.destBlockId = ''
        c.sourceBlockId = ''
    for b in description.blocks:
        b.blockGuid = ''
    description.blocks.sort(key=lambda b: b.blockCode)
    description.connections.sort(key=lambda c: (c.sourceBlockCode, c.sourceEndpointName, c.destBlockCode, c.destEndpointName))
    return description


class TestBase(object):
    @classmethod
    def setup_class(cls):
        cls.api = NirvanaApi('ff202eab4fb941f5b935806d2126e25e')

        root = ThirdBlock(
            code='3',
            tsv=SecondBlock(
                code='2',
                json=FirstBlock(code='1').outputs.json,
            ).outputs.tsv,
        )
        cls.workflow = create_workflow(cls.api, 'Nirvana Api Test', root)
        cls.canonical_desc = json.dumps(fix_workflow_description(cls.workflow.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)

    @classmethod
    def teardown_class(cls):
        method = 'deleteProcessDefinition'
        url = 'https://{0}/api/front/{1}'.format(cls.api.server, method)
        data = json_rpc.serialize(method, dict(processDefinitionId=cls.workflow.id), request_id=5)
        headers = {'Authorization': 'OAuth {0}'.format(cls.api.oauth_token), 'Content-Type': 'application/json;charset=utf-8'}
        response = requests.post(url, headers=headers, data=data, timeout=120)
        logger.info('Remove {}'.format(json_rpc.deserialize(response.text)))


class FirstBlock(BaseBlock):
    name = 'First block'
    guid = '82cedeb5-0544-11e6-a29e-0025909427cc'
    output_names = ['json', 'binary']
    parameters = ['test_str', 'test_int', 'test_bool', 'test_enum', 'test_str_list', 'test_int_list']


class SecondBlock(BaseBlock):
    name = 'Second block'
    guid = '491bc8db-0549-11e6-a29e-0025909427cc'
    input_names = ['json', 'binary', 'text']
    output_names = ['tsv']


class ThirdBlock(BaseBlock):
    name = 'Third block'
    guid = '8b54ad86-0545-11e6-a29e-0025909427cc'
    input_names = ['json', 'binary', 'tsv']


class FourthBlock(BaseBlock):
    name = 'Fourth block'
    guid = '0434dd95-054f-11e6-a29e-0025909427cc'
    input_names = ['one_name']
    output_names = ['one_name']
    parameters = ['one_name']


class FifthBlock(BaseBlock):
    name = 'FifthBlock'
    guid = '0434dd95-054f-11e6-a29e-0025909427cc'
    input_names = ['one_name']
    output_names = ['one_name']
    parameters = ['one_name']
    name_aliases = {
        'one_name': ['OneName', 'oneName']
    }
    is_strict = True


@singleton_block(join_params=['test_str'])
class SingletonFirstBlock(FirstBlock):
    pass
