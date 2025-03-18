from library.python.nirvana_api.test_lib import (
    NirvanaApiMock, BlockCodeMock)
import pytest
from nirvana_api.highlevel_api import create_workflow
from nirvana_api.blocks import BaseBlock
import nirvana_api.blocks as blocks
import tempfile
import yatest.common


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


@pytest.yield_fixture
def nirvana_api():
    with tempfile.NamedTemporaryFile(delete=False) as out:
        yield NirvanaApiMock(output=out)


@pytest.fixture
def mock(monkeypatch):
    monkeypatch.setattr(
        blocks.base_block,
        'get_block_code', BlockCodeMock().get_block_code
    )


def create_workflow_for_test(nirvana_api, block):
    create_workflow(nirvana_api, 'test', block, description='test description')
    nirvana_api.output.flush()
    return yatest.common.canonical_file(nirvana_api.output.name, local=True)


def test_execute_after(nirvana_api, mock):
    b = FirstBlock()
    SecondBlock(execute_after=b)
    return create_workflow_for_test(nirvana_api, b)


def test_execute_after_traverse(nirvana_api, mock):
    b = FirstBlock()
    bb = SecondBlock(execute_after=b)
    return create_workflow_for_test(nirvana_api, bb)
