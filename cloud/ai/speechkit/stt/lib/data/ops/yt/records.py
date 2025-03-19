from cloud.ai.lib.python.datasource.yt.model import generate_attrs
from cloud.ai.lib.python.datasource.yt.ops import TableMeta
from cloud.ai.speechkit.stt.lib.data.model.dao import Record, RecordAudio
from .common import get_tables_dir

table_records_meta = TableMeta(
    dir_path=get_tables_dir('records'),
    attrs=generate_attrs(
        Record,
        required={'received_at'},
    ),
)

table_records_audio_meta = TableMeta(
    dir_path=get_tables_dir('records_audio'),
    attrs=generate_attrs(
        RecordAudio,
        required={'hash', 'hash_version'},
    ),
)
