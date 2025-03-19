"""Handlers about cloud stats"""
import logging
from lib import Mongo

MONGOCLI = Mongo()

def search():
    """Get cloud info"""
    log = logging.getLogger()
    log.debug("Get cloud info")
    return MONGOCLI.cloud_info()
