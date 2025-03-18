"""
Internal API for proxy interaction
"""

from sqlalchemy.orm import load_only
from voluptuous import Optional, Coerce
from flask import Blueprint, jsonify

from antiadblock.configs_api.lib.context import CONTEXT
from antiadblock.configs_api.lib.db import AuditEntry, Config

from antiadblock.configs_api.lib.auth.nodes import Root
from antiadblock.configs_api.lib.utils import check_args
from antiadblock.configs_api.lib.const import ROOT_LABEL_ID
from antiadblock.configs_api.lib.auth.permissions import get_user_role_weight, check_permissions_user_or_service
from antiadblock.configs_api.lib.auth.permission_models import PermissionKind, DEFAULT_ADMIN_USERNAME, ROLES_WEIGHT

audit_api = Blueprint('audit_api', __name__)


@audit_api.route('/audit/service/<string:service_id>')
def get_service_audit(service_id):
    user_id = check_permissions_user_or_service(service_id, PermissionKind.SERVICE_SEE)
    if user_id:
        self_role_weight = get_user_role_weight([user_id], Root.SERVICES[service_id]).get(user_id, ROLES_WEIGHT["external_user"])
    else:
        self_role_weight = ROLES_WEIGHT['admin']

    args = check_args({Optional("offset", default=0): Coerce(int),
                       Optional("limit", default=20): Coerce(int),
                       Optional("label_id", default=""): basestring})
    if not args["label_id"]:
        labels = [service_id]
        label_id = service_id
        while label_id != ROOT_LABEL_ID:
            config = Config.query.options(load_only("parent_label_id")).filter_by(label_id=label_id).first()
            if not config:
                break
            label_id = config.parent_label_id
            labels.append(label_id)
    else:
        labels = [args["label_id"]]

    # All changes for service_id and parents labels without service_id
    query = AuditEntry.query.filter(AuditEntry.label_id.in_(labels)).order_by(AuditEntry.date.desc())

    overall = query.count()
    items = query.limit(args["limit"]).offset(args["offset"])

    result = [audit_entry.as_dict() for audit_entry in items.all()]
    user_ids = {a["user_id"] for a in result if a["user_id"]}
    user_logins = CONTEXT.blackbox.get_user_logins(user_ids)
    user_role_weights = get_user_role_weight(user_ids, Root.SERVICES[service_id])
    for audit_entry in result:
        if not audit_entry["user_id"]:
            audit_entry["user_name"] = "Mr. Robot"
        elif self_role_weight >= user_role_weights.get(audit_entry["user_id"]):
            audit_entry["user_name"] = user_logins.get(audit_entry["user_id"])
        else:
            audit_entry["user_name"] = DEFAULT_ADMIN_USERNAME
    return jsonify(dict(items=result, total=overall))
