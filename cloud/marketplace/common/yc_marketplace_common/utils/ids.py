from yc_common import config
from yc_common import ids
from yc_common.validation import ResourceIdType


def generate_id() -> ResourceIdType:
    return ids.generate_id(config.get_value("marketplace.id_prefix"), validate_prefix=True)
