"""Custom exceptions."""
import logging
from functools import wraps

from flask import jsonify, make_response, request

# pylint: disable=invalid-name
logger = logging.getLogger(__name__)


class IdmServiceError(Exception):
    """All idm-service errors."""


class NotImplementedError(IdmServiceError):
    """Method is not implemented."""


class DatabaseError(IdmServiceError):
    """Database related error."""


class ClusterNotExistsError(IdmServiceError):
    """Required cluster wasn't found."""


class UnsupportedClusterType(IdmServiceError):
    """Cluster type is not supported."""

    def __init__(self, cluster_type):
        super().__init__('Cluster type {} is not supported'.format(cluster_type))


class ClusterStatusNotRunningError(IdmServiceError):
    """Cluster status wasn't RUNNING."""


class UnsupportedClusterError(IdmServiceError):
    """Cluster is not managed by IDM."""


class InvalidUserOriginError(IdmServiceError):
    """User is not managed by IDM."""


def handle_errors(fun):
    """Handle internal errors."""

    @wraps(fun)
    def wrapper(*args, **kwargs):
        """Return code and message on error."""
        response = {'code': 1}
        code = 500
        path = request.path
        try:
            return fun(*args, **kwargs)
        except DatabaseError:
            logger.exception('DB error on path %s args: %s', path, kwargs)
            response['error'] = 'Database error'
            code = 500
        except ClusterNotExistsError:
            logger.exception('Cluster error on path %s args: %s', path, kwargs)
            response['error'] = 'Cluster does not exists'
            code = 400
        except ClusterStatusNotRunningError as error:
            logger.exception('Cluster error on path %s args: %s', path, kwargs)
            logger.exception(error)
            response['error'] = 'Cluster must be in status RUNNING'
            code = 400
        except UnsupportedClusterError:
            logger.exception('Cluster error on path %s args: %s', path, kwargs)
            response['error'] = 'IDM integration is disabled for this cluster'
            code = 400
        except InvalidUserOriginError:
            logger.exception('Cluster error on path %s args: %s', path, kwargs)
            response[
                'error'
            ] = 'User {login} is not managed by IDM. Remove user manually and IDM will create it'.format(
                login=kwargs.get('login')
            )
            code = 400
        return make_response(jsonify(response), code)

    return wrapper
