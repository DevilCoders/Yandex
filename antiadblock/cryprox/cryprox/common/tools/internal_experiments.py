# coding=utf8

import os
import json

from antiadblock.cryprox.cryprox.service.jslogger import JSLogger
from antiadblock.cryprox.cryprox.config.service import BYPASS_UIDS_FILE_PATH as BASE_FILE_PATH
from antiadblock.cryprox.cryprox.config.system import INTERNAL_EXPERIMENT_CONFIG
from antiadblock.cryprox.cryprox.common.tools.bypass_by_uids import Singleton

logger = JSLogger('root')


class InternalExperiment:
    """
    Конфиг для экспериментов приезжает из последнего SB-ресурса типа ANTIADBLOCK_CRYPROX_INTERNAL_EXPERIMENT_CONFIG
    Раз в минуту ходим в API Sandbox https://sandbox.yandex-team.ru/api/v1.0/resource?limit=1&type=ANTIADBLOCK_CRYPROX_INTERNAL_EXPERIMENT_CONFIG&state=READY&owner=ANTIADBLOCK
    Если последний ресурс изменился, то скачивается с помощью `sky get` в скрипте `antiadblock/cryprox/update_resources.py`
    Посмотреть список ресурсов в UI можно так https://sandbox.yandex-team.ru/resources?type=ANTIADBLOCK_CRYPROX_INTERNAL_EXPERIMENT_CONFIG&limit=20
    """
    __metaclass__ = Singleton

    def __init__(self):
        self.config = {}
        self.last_update_inode = None

    def update_config(self):
        filename = os.path.join(BASE_FILE_PATH, INTERNAL_EXPERIMENT_CONFIG)
        if os.path.exists(filename):
            inode = os.stat(filename).st_ino
            if inode != self.last_update_inode:
                try:
                    with open(filename) as fin:
                        self.config = json.load(fin)
                    self.last_update_inode = inode
                    logger.info(None, action='internal_experiment_config_update', status='success', filename=filename)
                except Exception:
                    self.config = {}
                    self.last_update_inode = None
                    logger.error(None, action='internal_experiment_config_update', status='fail', filename=filename)

    def get_exp_id(self, service_id):
        return self.config.get(service_id)
