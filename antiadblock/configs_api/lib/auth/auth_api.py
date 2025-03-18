from flask import Blueprint, jsonify
from antiadblock.configs_api.lib.auth.nodes import ROOT
from antiadblock.configs_api.lib.context import CONTEXT

from antiadblock.configs_api.lib.auth.permissions import get_user_permissions

auth_api = Blueprint('auth_api', __name__)


@auth_api.route('/auth/permissions/global')
def get_global_permissions():
    user_id = CONTEXT.auth.get_user_id()
    permissions = get_user_permissions(user_id).get_permissions_list(ROOT)
    return jsonify(dict(user_id=user_id, permissions=list(permissions)))


@auth_api.route('/auth/permissions/service/<string:service_id>')
def get_service_permissions(service_id):
    user_id = CONTEXT.auth.get_user_id()
    permissions = get_user_permissions(user_id).get_permissions_list(ROOT.SERVICES[service_id])
    return jsonify(dict(user_id=user_id, permissions=list(permissions)))
