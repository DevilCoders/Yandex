from .base_block import BaseBlock
from .processor_parameters import ProcessorType


class SvnCheckoutFile(BaseBlock):
    guid = 'dab96a8d-235c-11e6-a29e-0025909427cc'
    parameters = ['path', 'revision']
    output_names = ['file']
    processor_type = ProcessorType.Job


class SvnCheckoutDirectory(BaseBlock):
    guid = '65a17304-b25f-11e6-98ff-0025909427cc'
    name = 'svn checkout deterministic'
    parameters = ['url', 'revision', 'depth', 'ignore-externals']
    output_names = ['working_directory']
    processor_type = ProcessorType.Job


class SvnCheckout(BaseBlock):
    guid = 'c5d811fa-4786-4820-90c9-4224f2b66751'
    name = 'SVN checkout'
    parameters = ['arcadia_path', 'revision', 'depth', 'ignore-externals',
                  'path_prefix', 'timestamp']
    output_names = ['archive', 'revision']


class SvnExportOneFile(BaseBlock):
    """This is deterministic operation"""

    guid = '77abd1ef-57fb-48a9-a39d-c340206c0964'
    name = 'SVN export one file'
    parameters = ['arcadia_path', 'revision', 'only_info',
                  'path_prefix', 'timestamp']
    output_names = ['text', 'json', 'binary', 'executable', 'tsv', 'file',
                    'revistion']


class SvnExportSingleFile(BaseBlock):
    """This is non-deterministic operation"""

    guid = '7306fbe8-3ae7-4eb0-b846-e132649b9033'
    name = 'SVN: Export single file'
    parameters = ['arcadia_path', 'revision', 'only_info', 'path_prefix']
    output_names = ['text', 'json', 'binary', 'executable', 'tsv', 'file',
                    'revistion']
