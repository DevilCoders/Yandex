from cloud.ai.lib.python.datasource.yt.model import generate_attrs
from cloud.ai.lib.python.datasource.yt.ops import TableMeta
from cloud.ai.speechkit.stt.lib.data.model.dao import MarkupParams
from .common import get_tables_dir

table_markups_params_meta = TableMeta(
    dir_path=get_tables_dir('markups_params'),
    attrs=generate_attrs(
        MarkupParams,
        required={'received_at'},
    ),
)
