import yaml
from dataclasses import dataclass
from datetime import datetime


class LMSLoadType:
    Full = "full"
    Incremental = "incremental"
    File = "file"


class LMSIncrementType:
    Date = "date"
    Datetime = "datetime"
    Integer = "integer"


@dataclass
class FileListRecord:
    path: str
    size: int
    modified_dttm: datetime


class LMSDBLoadMetadataInc(yaml.YAMLObject):
    yaml_tag = "!DBLoadMetadataInc"

    def __init__(self,
                 object_id: str,
                 load_type: str,
                 increment_type: str,
                 increment_column_name: str,
                 step_back_value: str,
                 ):
        self.object_id = object_id
        self.load_type = load_type
        self.increment_column_name = increment_column_name
        self.increment_type = increment_type
        self.step_back_value = step_back_value

    def __repr__(self):
        return "LMSDBLoadMetadataInc(" + str(vars(self)) + ")"


class LMSDBLoadMetadataFull(yaml.YAMLObject):
    yaml_tag = "!DBLoadMetadataFull"

    def __init__(self,
                 object_id: str,
                 load_type: str,
                 ):
        self.object_id = object_id
        self.load_type = load_type

    def __repr__(self):
        return "LMSDBLoadMetadataFull(" + str(vars(self)) + ")"
