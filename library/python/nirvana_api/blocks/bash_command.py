from .base_block import BaseBlock
from .processor_parameters import ProcessorType


class BashCommand(BaseBlock):
    guid = 'ed688f78-98ff-11e5-8ed5-0025909427cc'
    parameters = ['cmd']
    input_names = ['input']
    output_names = ['output']
    processor_type = ProcessorType.Job


class RunBinaryFilter(BaseBlock):
    guid = '0ed3d673-0560-11e7-a873-0025909427cc'
    parameters = ['args']
    input_names = ['executable', 'input']
    output_names = ['output']
    processor_type = ProcessorType.Job


class RunTextFilter(BaseBlock):
    guid = '10910f3e-055f-11e7-a873-0025909427cc'
    parameters = ['args']
    input_names = ['executable', 'input']
    output_names = ['output']
    processor_type = ProcessorType.Job


class TsvSort(BaseBlock):
    guid = '2b7d7585-29a0-11e7-89a6-0025909427cc'
    input_names = ['files']
    output_names = ['sorted']
    parameters = ['key', 'unique', 'stable', 'locale']
