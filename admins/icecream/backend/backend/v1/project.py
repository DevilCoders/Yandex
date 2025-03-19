"""Projects info handlers"""
import logging
import flask
from lib import Mongo


MONGOCLI = Mongo()
LOG = logging.getLogger()


def search():
    """Return all projects info"""
    LOG.debug("Get projects info")
    info = MONGOCLI.projects_info()
    if not info:
        flask.abort(500, "Failed to build projects info")
    return info


def get(project):
    """Return specific project info"""
    LOG.debug("Query about project: %s", project)

    info = MONGOCLI.projects_info(project)
    if not info:
        flask.abort(500, "Failed to build project info for {}".format(project))
    return info
