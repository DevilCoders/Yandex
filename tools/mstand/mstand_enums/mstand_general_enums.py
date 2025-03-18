from yaqutils import YaqEnum


class YtResourceTypeEnum(YaqEnum):
    DISK_SPACE = "disk_space"
    CHUNK_COUNT = "chunk_count"
    NODE_COUNT = "node_count"


class YtDownloadingFormats(YaqEnum):
    JSON = "json"
    YSON = "yson"
    ALL = {JSON, YSON}


class YsonFormats(YaqEnum):
    BINARY = "binary"
    TEXT = "text"
    PRETTY = "pretty"
    ALL = {BINARY, TEXT, PRETTY}


class SqueezeWayEnum(YaqEnum):
    BINARY = "binary"
    NILE = "nile"
    YQL = "yql"
    YT = "yt"
    ALL = {BINARY, NILE, YQL, YT}
