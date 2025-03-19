import os.path

from yc_common import config
from cloud.marketplace.common.yc_marketplace_common.utils.dev_config import dev_config


def load(devel_mode=False):
    cfg_path = os.getenv("YC_CONFIG_PATH")
    if cfg_path is not None:
        config.load([cfg_path], ignore_unknown=True)
    else:
        builtin_configs_dir_path = os.path.dirname(__file__)
        default_path = dev_config(builtin_configs_dir_path) if devel_mode else "/etc/yc-marketplace/api-config.yaml"
        config.load([default_path], ignore_unknown=True)
