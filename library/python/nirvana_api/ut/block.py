from nirvana_api.blocks import BaseBlock
from nirvana_api.blocks.processor_parameters import ProcessorType
import pytest


class SomeBlock(BaseBlock):
    processor_type = ProcessorType.Job
    guid = '123'
    parameters = ['a']
    inputs = ['x']


def test_fail_create_unknown_parameter():
    with pytest.raises(AssertionError):
        block = SomeBlock(ya='123')  # noqa


def test_accept_known_parameters():
    block = SomeBlock(a='123', max_ram=300)  # noqa
