from .base_block import BaseBlock
from .processor_parameters import ProcessorType


class FileConverter(BaseBlock):
    input_names = ['input_file']
    output_names = ['output_file']
    processor_type = ProcessorType.Job


class FileToBinaryData(FileConverter):
    guid = 'ad9b38bd-f1b4-11e5-bdc7-0025909427cc'


class BinaryDataToText(FileConverter):
    guid = '4b2cee8a-023e-11e7-a873-0025909427cc'


class FileToExecutable(FileConverter):
    guid = 'd9110c38-f1b4-11e5-bdc7-0025909427cc'


class FileToHtml(FileConverter):
    guid = 'e64fe4fd-f1b5-11e5-bdc7-0025909427cc'


class FileToImage(FileConverter):
    guid = 'f8071464-f1b5-11e5-bdc7-0025909427cc'


class FileToJson(FileConverter):
    guid = '0827da89-f1b6-11e5-bdc7-0025909427cc'


class ToJson(FileConverter):
    guid = '14c3d83b-51dd-4960-b5f2-125ee1fdc9b1'


class FileToMRTable(FileConverter):
    guid = '212f1175-f1b6-11e5-bdc7-0025909427cc'


class FileToText(FileConverter):
    guid = '31901f00-f1b6-11e5-bdc7-0025909427cc'


class FileToTsv(FileConverter):
    guid = 'db0607a6-f1b3-11e5-bdc7-0025909427cc'


class FileToXml(FileConverter):
    guid = '4198b677-f1b6-11e5-bdc7-0025909427cc'


class ToTsvFile(FileConverter):
    guid = '87e043d2-2752-11e6-a29e-0025909427cc'


class ToBinaryData(FileConverter):
    guid = 'd1b300ff-28ef-11e7-89a6-0025909427cc'


class GzipCompress(FileConverter):
    guid = 'b65a7471-0ef1-11e7-a873-0025909427cc'
