"""Queue configuration"""

import os.path

from cloud.marketplace.common.yc_marketplace_common.utils.dev_config import dev_config
from yc_common import config
from yc_common.models import IntType
from yc_common.models import Model
from yc_common.models import ModelType
from yc_common.models import StringType


class LogbrokerTVMConfig(Model):
    client_id = IntType(default=os.getenv("LOGBROKER_TVM_CLIENT_ID"))
    secret = StringType(default=os.getenv("LOGBROKER_TVM_SECRET"))
    destination = IntType(required=True)


class LogbrokerConfig(Model):
    host = StringType(required=True)
    port = IntType(required=True)
    auth = ModelType(LogbrokerTVMConfig, required=True)
    topic_template = StringType(required=True)


def load(devel_mode=False):
    cfg_path = os.getenv("YC_CONFIG_PATH")
    if cfg_path is not None:
        config.load([cfg_path], ignore_unknown=True)
    else:
        builtin_configs_dir_path = os.path.dirname(__file__)
        default_path = dev_config(builtin_configs_dir_path) if devel_mode else "/etc/yc-marketplace/queue-config.yaml"
        config.load([default_path], ignore_unknown=True)
