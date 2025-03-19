from yc_common.clients.kikimr.client import KIKIMR_CLIENT_YDB
from yc_common.clients.kikimr.client import client_version

from yc_common.exceptions import LogicalError

from yc_common.models import DECIMAL_PRECISION, DECIMAL_SCALE


# FIXME: add container KiKiMR data types
class KikimrDataType:
    """
    Kikimr available data types
    See: https://ydb.yandex-team.ru/docs/concepts/datatypes/
    """
    # Numeric types
    BOOL = "Bool"
    INT8 = "Int8"
    INT16 = "Int16"
    INT32 = "Int32"
    INT64 = "Int64"
    UINT8 = "Uint8"
    UINT16 = "Uint16"
    UINT32 = "Uint32"
    UINT64 = "Uint64"
    FLOAT = "Float"
    DECIMAL = "Decimal({},{})".format(DECIMAL_PRECISION, DECIMAL_SCALE)
    DOUBLE = "Double"

    # String types
    STRING = "String"
    UTF8 = "Utf8"
    JSON = "Json"
    YSON = "Yson"
    UUID = "Uuid"

    # Date and time types
    DATE = "Date"
    DATETIME = "Datetime"
    TIMESTAMP = "Timestamp"
    INTERVAL = "Interval"
    TZDATE = "TzDate"
    TZDATETIME = "TzDatetime"
    TZTIMESTAMP = "TzTimestamp"

    # Null types
    NULL = "Null"

    ALL = [
        BOOL, INT8, INT16, INT32, INT64, UINT8, UINT16, UINT32, UINT64, FLOAT, DECIMAL, DOUBLE,
        STRING, UTF8, JSON, YSON, UUID,
        DATE, DATETIME, TIMESTAMP, INTERVAL, TZDATE, TZDATETIME, TZTIMESTAMP,
        NULL,
    ]


def table_spec_type_to_internal_type(type: "ydb.public.api.protos.ydb_value_pb2.Type"):
    type_name = type.__str__()  # type: str
    type_name = type_name.rstrip("?")
    if type_name.startswith("Decimal(") and client_version() != KIKIMR_CLIENT_YDB:
        type_name = "`{}`".format(type_name)

    if type_name not in KikimrDataType.ALL:
        raise LogicalError("Unknown data type: '{}'", type)
    return type_name
