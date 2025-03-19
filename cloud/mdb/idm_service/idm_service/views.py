"""API Resources definition."""
import logging
from collections import defaultdict
from enum import auto, Enum
from functools import partial, wraps

from flask import jsonify, request
from flask_restful import Resource
from tvm2 import TVM2
from webargs.flaskparser import abort, parser, use_kwargs

from .exceptions import DatabaseError, handle_errors
from .metadb import DB
from .schemas import GrantRequestSchema
from .tvm_constants import TVM_CONSTANTS
from .utils import GRANTS, is_hiding

# pylint: disable=invalid-name
logger = logging.getLogger(__name__)


class TVM2ClientType(Enum):
    idm = auto()
    soc = auto()


def get_tvm2_client(client_type=TVM2ClientType.idm):
    """Method to create tvm2 client or get existed one"""
    client = {
        TVM2ClientType.idm: TVM_CONSTANTS.idm_client_id,
        TVM2ClientType.soc: TVM_CONSTANTS.soc_client_id,
    }[client_type]

    return TVM2(
        client_id=TVM_CONSTANTS.client_id,
        secret=TVM_CONSTANTS.client_secret,
        blackbox_client=TVM_CONSTANTS.blackbox_client,
        allowed_clients=(client,),
    )


def validate_service_ticket(f, client_type=TVM2ClientType.idm):
    """Decorator to validate service ticket"""

    @wraps(f)
    def wrapper(*args, **kw):
        if not TVM_CONSTANTS.enabled:
            return f(*args, **kw)
        tvm_service_ticket = request.headers.get('X-Ya-Service-Ticket')
        if tvm_service_ticket is None:
            return abort(403, errors="ServiceTicketNotFound: Header X-Ya-Service-Ticket not found")
        parsed_ticket = get_tvm2_client(client_type).parse_service_ticket(tvm_service_ticket)
        if parsed_ticket is None:
            return abort(403, errors="InvalidServiceTicket: Value X-Ya-Service-Ticket is invalid")
        return f(*args, **kw)

    return wrapper


validate_soc_service_ticket = partial(validate_service_ticket, client_type=TVM2ClientType.soc)


@parser.error_handler
def handle_request_parsing_error(err):
    """Log validation errors."""
    logger.error('Invalid request: %s', err.messages)
    abort(422, errors=err.messages)


class Info(Resource):
    """Get info about the system and available roles."""

    @validate_service_ticket
    @handle_errors
    def get(self):
        """Get available roles."""
        response = {
            'code': 0,
            'roles': {
                'slug': 'cluster',
                'name': {
                    'ru': 'Кластер',
                    'en': 'Cluster',
                },
                'values': {},
            },
        }
        data = DB.get_clusters()
        for cluster in data:
            cid = cluster.cid
            response['roles']['values'][cid] = {
                'name': {
                    'ru': cluster.name,
                    'en': cluster.name,
                },
                'help': {
                    'ru': cid,
                    'en': cid,
                },
                'roles': {
                    'name': {
                        'ru': 'уровень доступа',
                        'en': 'access privileges',
                    },
                    'slug': 'grants',
                    'values': GRANTS,
                },
            }
        return jsonify(response)


class AddRole(Resource):
    """Grant role to the user."""

    @validate_service_ticket
    @use_kwargs(GrantRequestSchema(strict=True))
    @handle_errors
    def post(self, login, role):
        """Grant role."""
        cid = role['cluster']
        grant = role['grants']
        if grant == 'responsible':
            DB.add_resp(login, cid)
        else:
            DB.grant_role(login, cid, grant)
        return {'code': 0}


class RemoveRole(Resource):
    """Revoke role from the user."""

    @validate_service_ticket
    @use_kwargs(GrantRequestSchema(strict=True))
    @handle_errors
    def post(self, login, role):
        """Revoke role."""
        cid = role['cluster']
        grant = role['grants']
        if grant == 'responsible':
            DB.remove_resp(login, cid)
        else:
            DB.revoke_role(login, cid, grant)
        return {'code': 0}


class AllRoles(Resource):
    """Get the list of all users and their roles."""

    @validate_service_ticket
    @handle_errors
    def get(self):
        """Get all roles."""
        response = {
            'code': 0,
            'users': [],
        }
        data = DB.get_clusters_roles()
        users = defaultdict(list)
        for (cid, _), cinfo in data.items():
            for login, grants in cinfo.items():
                cgrants = [
                    {
                        'cluster': cid,
                        'grants': grant,
                    }
                    for grant in grants
                ]
                users[login].extend(cgrants)
        for login, grants in users.items():
            response['users'].append(
                {
                    'login': login,
                    'roles': grants,
                }
            )
        return jsonify(response)


class AllClusters(Resource):
    """Get the list of all IDM clusters."""

    @validate_soc_service_ticket
    @handle_errors
    def get(self):
        """Get all IDM clusters."""
        clusters = [{'cid': c.cid, 'name': c.name, 'folder_id': c.folder_id} for c in DB.get_clusters()]
        return {'code': 0, 'clusters': clusters}


class RotatePasswords(Resource):
    """Rotate all stale passwords."""

    @handle_errors
    def get(self):
        """Rotate passwords."""
        DB.rotate_passwords()
        return {'code': 0}


class Ping(Resource):
    """Ping the service."""

    def get(self):
        """Check the database connection."""
        if is_hiding():
            abort(503, message='Hide flag')
        try:
            DB.ping()
            return 'ok'
        except DatabaseError:
            abort(503, message='No database connection')


def init_routes(api):
    """Register the views."""
    api.add_resource(Ping, '/ping')
    api.add_resource(Info, '/info/')
    api.add_resource(AddRole, '/add-role/')
    api.add_resource(RemoveRole, '/remove-role/')
    api.add_resource(AllRoles, '/get-all-roles/')
    api.add_resource(AllClusters, '/get-all-clusters/')
    api.add_resource(RotatePasswords, '/rotate-passwords/')
