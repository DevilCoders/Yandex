from .base_block import BaseBlock
from .processor_parameters import ProcessorType


class GetSandboxResource(BaseBlock):
    guid = '20004369-27de-11e6-a29e-0025909427cc'
    name = 'Get sandbox resource'
    parameters = ['resource_id', 'do_pack']
    output_names = ['resource']
    processor_type = ProcessorType.Job


class FindSandboxResource(BaseBlock):
    guid = 'f1290a9d-4522-11e7-89a6-0025909427cc'
    name = 'Find sandbox resource'
    input_names = ['dependency']
    output_names = ['result']
    parameters = ['resource_type', 'attrs', 'latest', 'order', 'checkout_arcadia_from_url', 'timestamp', 'state']


class UploadToSandbox(BaseBlock):
    guid = '5033919c-921e-4db6-8469-33ec3414ea8d'
    input_names = ['upload_file']
    output_names = ['resource_url', 'resource_id']
    parameters = ['sandbox_type', 'sandbox_token', 'sandbox_ttl', 'sandbox_owner',
                  'sandbox_arch', 'sandbox_description', 'timestamp', 'untar']
    processor_type = ProcessorType.Job


class SkyGet(BaseBlock):
    guid = '2eaa2c08-31a6-42e5-9d71-4f9841fa93cb'
    name = 'SkyGet'
    input_names = ['rbtorrent', 'sbr']
    output_names = ['file']
    parameters = ['rbtorrent']
    processor_type = ProcessorType.Job
