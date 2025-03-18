from .base_block import BaseBlock
from .processor_parameters import ProcessorType


class GunzipTsv(BaseBlock):
    guid = '74e7c67c-0168-11e6-a29e-0025909427cc'
    name = 'Gunzip .tsv.gz'
    input_names = ['tsv.gz']
    output_names = ['tsv']
    processor_type = ProcessorType.Job


class AddToTar(BaseBlock):
    guid = 'ba3ede53-84ac-412e-be60-4510e3035d54'
    name = 'Add to tar'
    input_names = ['archive', 'file']
    output_names = ['archive']
    parameters = ['path']
    processor_type = ProcessorType.Job


class Concatenate(BaseBlock):
    guid = 'f46fe445-0ef3-11e7-a873-0025909427cc'
    input_names = ['src_part_file']
    output_names = ['result_file']
    processor_type = ProcessorType.Job


class JQ(BaseBlock):
    guid = '1a1a3a44-fd02-11e6-a873-0025909427cc'
    name = 'jq'
    input_names = ['src']
    output_names = ['json', 'tsv']
    parameters = ['script']
