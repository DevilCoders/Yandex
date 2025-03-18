# coding: utf-8
import yenv
import statface_client

STATFACE_HOST = statface_client.STATFACE_BETA
if yenv.type == 'production':
    STATFACE_HOST = statface_client.STATFACE_PRODUCTION

METRIC_SLUG_MAPPING = {}

METRIC_MIN_LOCK_TIME = 5

TIME_ZONE='Europe/Moscow'
