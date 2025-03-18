# -*- encoding: utf-8 -*-
from __future__ import unicode_literals

import logging


def configure_logging(level=logging.INFO):
    """Конфигурирует протоколирование.

    :param level: Минимальный уровень важности записей.

    """
    logging.basicConfig(level=level, format='%(levelname)s: %(message)s')
