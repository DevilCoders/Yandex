from .base_block import BaseBlock
from .processor_parameters import ProcessorType


class Cluster(object):
    hahn = 'hahn'
    banach = 'banach'
    freud = 'freud'
    marx = 'marx'
    hume = 'hume'


class MRPathType(object):
    table = 'TABLE'
    directory = 'DIRECTORY'
    file = 'FILE'


class CreationMode(object):
    no_check = 'NO_CHECK'
    check_exists = 'CHECK_EXISTS'


class WriteMode(object):
    overwrite = 'OVERWRITE'
    append = 'APPEND'


class MergeMode(object):
    auto = 'AUTO'
    unordered = 'UNORDERED'
    ordered = 'ORDERED'
    sorted = 'SORTED'


class GetMRTable(BaseBlock):
    guid = '6ef6b6f1-30c4-4115-b98c-1ca323b50ac0'
    name = 'Get MR Table'
    parameters = ['type', 'cluster', 'creationMode', 'table', 'yt-token']
    input_names = ['fileWithTableName', 'base_path']
    output_names = ['outTable']
    processor_type = ProcessorType.GetMRPath


class GetMRDirectory(BaseBlock):
    guid = 'eec37746-6363-42c6-9aa9-2bfebedeca60'
    name = 'Get MR Directory'
    parameters = ['cluster', 'creationMode', 'path', 'yt-token']
    input_names = ['path', 'base_path']
    output_names = ['mr_directory']
    processor_type = ProcessorType.GetMRPath


class MRRead(BaseBlock):
    guid = 'c623527a-63f2-11e6-a050-3c970e24a776'
    name = 'MR Read'
    parameters = [
        'start-row',
        'end-row',
        'has-subkey',
        'key-column',
        'value-column',
        'subkey-column',
        'timestamp',
    ]
    input_names = [
        'table',
        'sync',
    ]
    output_names = [
        'tsv',
    ]
    processor_type = ProcessorType.MR


class MRReadTsv(BaseBlock):
    guid = '3a47a5d7-d9c3-11e6-984a-3c970e24a776'
    name = 'MR Read TSV'
    input_names = ['table', 'sync']
    output_names = ['tsv']
    parameters = [
        'timestamp', 'columns', 'start-row', 'end-row',
        'missing-value-mode', 'missing-value-sentinel',
        'output-escaping', 'output-escaping-symbol'
    ]
    processor_type = ProcessorType.MR


class MRWrite(BaseBlock):
    guid = '99a9c2e5-9c83-11e6-8943-3c970e24a776'
    name = 'MR Write'
    parameters = [
        'write-mode',
        'has-subkey',
        'key-column',
        'value-column',
        'subkey-column',
        'timestamp',
    ]
    input_names = [
        'tsv',
        'dst_table',
        'sync',
    ]
    output_names = [
        'done',
    ]
    processor_type = ProcessorType.MR


class MRWriteCreate(BaseBlock):
    guid = '8392fb5a-9c34-11e6-8943-3c970e24a776'
    name = 'MR Write'
    parameters = [
        'has-subkey',
        'key-column',
        'value-column',
        'subkey-column',
        'timestamp',
    ]
    input_names = [
        'tsv',
        'dst_dir',
    ]
    output_names = [
        'new_table',
    ]
    processor_type = ProcessorType.MR


class MRMove(BaseBlock):
    guid = '14fae616-62f6-11e6-a050-3c970e24a776'
    parameters = ['force', 'timestamp']
    input_names = ['src', 'dst']
    output_names = ['moved']
    processor_type = ProcessorType.MR


class MRCopyTableToPath(BaseBlock):
    guid = '23762895-cf87-11e6-9372-6480993f8e34'
    parameters = [
        'dst-cluster',
        'dst-path',
        'force',
        'timestamp'
    ]
    input_names = ['src']
    output_names = ['copy']
    processor_type = ProcessorType.MR


class MRSort(BaseBlock):
    guid = '0db0596a-1d5c-11e7-904c-3c970e24a776'
    parameters = [
        'sort-by',
        'dst-path',
        'timestamp',
        'preserve-attributes',
    ]
    input_names = ['srcs']
    output_names = ['sorted']
    processor_type = ProcessorType.MR


class MRMerge(BaseBlock):
    guid = '2f511371-d5d1-11e6-9372-6480993f8e34'
    parameters = [
        'merge-mode',
        'append',
        'dst-path',
        'timestamp'
    ]
    input_names = ['srcs']
    output_names = ['merged']
    processor_type = ProcessorType.MR
