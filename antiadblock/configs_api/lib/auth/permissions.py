import re2

from flask import abort, g, current_app, request
from voluptuous import Schema

from antiadblock.configs_api.lib.auth.nodes import ROOT
from antiadblock.configs_api.lib.auth.permission_models import PermissionKind, DEFAULT_MASK, ROLES, DEFAULT_WEIGHT, ROLES_WEIGHT
from antiadblock.configs_api.lib.auth.auth import ensure_tvm_ticket_is_ok
from antiadblock.configs_api.lib.context import CONTEXT
from antiadblock.configs_api.lib.db import Permission, Service, db


def check_permissions_user_or_service(service_id, user_permission,
                                      allowed_service_apps=("AAB_SANDBOX_MONITORING_TVM_ID",
                                                            "AAB_CRYPROX_TVM_CLIENT_ID",
                                                            "ANTIADB_SUPPORT_BOT_TVM_CLIENT_ID")):
    """
    :param service_id:
    :param user_permission:
    :param allowed_service_apps:
    :return: user_id if user request or 0 if service request, raised exception if check permission failed
    """
    if request.headers.get("X-Ya-Service-Ticket"):
        ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=allowed_service_apps)
        user_id = 0
    else:
        user_id = ensure_permissions_on_service(service_id, user_permission)
    return user_id


def get_upper_nodes_tree(node):
    """
    returns list of upper permission nodes.
    :param node:
    :return:

    >>> get_upper_nodes_tree('*')
    ['*']

    >>> get_upper_nodes_tree('*#services#auto_ru')
    ['*', '*#services', '*#services#auto_ru']

    """

    nodes = []
    growing_node = ""
    for edge in node.split("#"):
        if growing_node:
            growing_node += "#"
        growing_node += edge
        nodes.append(growing_node)
    return nodes


def get_user_permissions(user_id):
    if user_id is None:
        return GUEST_PERMISSIONS

    if g.get("permissions") is not None:
        return g.permissions

    service_ids = []
    domains = CONTEXT.webmaster.get_user_domains()
    if domains:
        service_ids = db.session.query(Service.id).filter(Service.domain.in_(domains)).all()
    permissions_by_domains = Permissions([Permission(node=ROOT.SERVICES[service_id], role="external_user", uid=user_id) for (service_id,) in service_ids])
    g.permissions = permissions_by_domains.combine(Permissions(Permission.query.filter_by(uid=user_id).all()))
    return g.permissions


def ensure_permissions_on_service(service_id, permission):
    return _ensure_permission_on_node(permission=permission, node=ROOT.SERVICES[service_id] if service_id else ROOT)


def ensure_global_permissions(permission):
    return _ensure_permission_on_node(permission=permission, node=ROOT)


def _ensure_permission_on_node(permission, node):
    user_ticket = CONTEXT.auth.get_user_ticket()
    if not user_ticket:
        if not GUEST_PERMISSIONS.check_permission(node, permission):
            user_id = CONTEXT.auth.get_user_id_from_ticket(user_ticket)
            current_app.logger.warn("Permission {} check failed for guest on node {}".format(permission, user_id, node),
                                    extra=dict(permission=permission, user_id=user_id, node=str(node)))
            abort(403)
    user_id = CONTEXT.auth.get_user_id_from_ticket(user_ticket)
    if not get_user_permissions(user_id).check_permission(node, permission):
        current_app.logger.warn("Permission {} check failed for user {} on node {}".format(permission, user_id, node),
                                extra=dict(permission=permission, user_id=user_id, node=str(node)))
        abort(403)
    return user_id


def ensure_permissions_on_domain(domain):
    user_id = CONTEXT.auth.get_user_id()
    if not user_id:
        abort(403)
    # TODO: check permissions
    return user_id


def ensure_user_admin():
    ensure_global_permissions(PermissionKind.SERVICE_CREATE)


def get_user_role_weight(user_ids, node):
    """
    :param user_ids:
    :param node:
    :return: {user_id: role_weight, ...}
    """
    if not user_ids or not node:
        return {}

    role_weights = {}

    nodes = [str(item) for item in node.get_upper_nodes()]
    roles = Permission.query.filter((Permission.uid.in_(user_ids)) & (Permission.node.in_(nodes))).all()
    if len(roles) == 0:
        return {}
    for next_role in roles:
        if ROLES_WEIGHT.get(next_role.role, DEFAULT_WEIGHT) >= role_weights.get(next_role.uid, DEFAULT_WEIGHT):
            role_weights[next_role.uid] = ROLES_WEIGHT.get(next_role.role, DEFAULT_WEIGHT)
    return role_weights


class Permissions(object):
    def __init__(self, permissions):
        self.permissions = Schema([Permission])(permissions)

    def __iter__(self):
        return self.permissions.__iter__()

    def check_permission(self, node, action):
        if len(self.permissions) == 0:
            return GUEST_PERMISSIONS.check_permission(node, action)
        actual_permissions = self._get_actual_permissions(node)
        return reduce(lambda base, p: base or action in ROLES.get(p.role, DEFAULT_MASK), actual_permissions, False)

    def get_permitted_services(self):
        if self.check_permission(ROOT.SERVICES, PermissionKind.SERVICE_SEE):
            return db.session.query(Service.id).all()
        services = []
        service_re = re2.compile("\*#services#(.+)")
        for permission in self.permissions:
            service_node_match = service_re.match(str(permission.node))
            if service_node_match and PermissionKind.SERVICE_SEE in ROLES.get(permission.role, DEFAULT_MASK):
                services.append(service_node_match.group(1))
        return services

    def _get_actual_permissions(self, node):
        if len(self.permissions) == 0:
            return GUEST_PERMISSIONS._get_actual_permissions(node)
        actual_nodes = node.get_upper_nodes()
        return sorted(filter(lambda p: p.node in actual_nodes, self.permissions), key=lambda p: len(p.node))

    def get_permissions_list(self, node):
        return reduce(lambda b, p: b | ROLES[p.role], self._get_actual_permissions(node), set())

    def combine(self, another_permissions):
        if another_permissions is None:
            return self
        return Permissions([p for p in self.permissions] + [p for p in another_permissions.permissions])


GUEST_PERMISSIONS = Permissions([Permission(uid=None, node=ROOT, role="guest")])
