# coding=utf-8

"""
API для интеграции админки с IDM (выдача ролей для системы)
https://st.yandex-team.ru/ANTIADB-832
Документация https://wiki.yandex-team.ru/intranet/idm/API/
"""

import json

from flask import Blueprint, request, abort, current_app, jsonify, g, make_response
from voluptuous import Schema, Required, Optional, Coerce

from antiadblock.configs_api.lib.audit.audit import log_action
from antiadblock.configs_api.lib.auth.nodes import ROOT
from antiadblock.configs_api.lib.context import CONTEXT
from antiadblock.configs_api.lib.db import Permission, db, UserLogins, Service

from antiadblock.configs_api.lib.auth.permission_models import IDM_ROLES, ROLES
from antiadblock.configs_api.lib.models import AuditAction

idm_api = Blueprint('idm_api', __name__)


def _ensure_idm_tvm_ticket_is_ok():
    CONTEXT.auth.validate_service_ticket(request.headers.get("X-Ya-Service-Ticket"), [current_app.config["IDM_TVM_CLIENT_ID"]])


def get_data(request_form):
    data = {key: request_form[key] for key in request_form.keys()}
    for key, value in data.iteritems():
        try:
            data[key] = json.loads(value)
        except:
            continue
    return data


def parse_args(args):
    services = ["*", "all"]
    services += [s.id for s in Service.query.order_by(Service.name).all()]
    role = args["role"]
    if role["project"] == "other":
        project = role.get("service")
    else:
        project = role["project"]

    if project not in services:
        msg = "There is no such project: {}".format(project)
        current_app.logger.warn(msg)
        abort(make_response(jsonify(dict(code=1, fatal="There is no such project")), 200))

    if project in ["*", "all"]:
        node = str(ROOT)
    else:
        node = str(ROOT.SERVICES[project])
    role = role["role"]
    if role not in ROLES:
        msg = "There is no such role: {}".format(role)
        current_app.logger.warn(msg)
        abort(make_response(jsonify(dict(code=1, fatal="There is no such role")), 200))

    passportlogin = args["fields"]["passport-login"]

    ids = CONTEXT.blackbox.get_user_ids([passportlogin])
    # yandex login in which " - "replaced by "." considered the same. And vice versa. So use idm version of passport login.
    uid = None
    if len(ids) == 1:
        uid = ids.items()[0][1]

    return uid, passportlogin, role, node


@idm_api.route('/idm/add-role/', methods=['POST'])
def add_role():
    _ensure_idm_tvm_ticket_is_ok()
    args = Schema({Required("login"): basestring,
                   Required("role"): dict,
                   Required("path"): basestring,
                   Required("fields"): dict})(get_data(request.form))

    uid, passportlogin, role, node = parse_args(args)

    if not uid:
        msg = "Bad passport login: {}".format(passportlogin)
        current_app.logger.warn(msg)
        abort(make_response(jsonify(dict(code=1, fatal="Bad passport login")), 200))

    # проверим, что пользователь не имеет роли на другой логин
    uids = UserLogins.query.filter_by(internallogin=args["login"]).all()
    if uids:
        if uid not in [u.uid for u in uids]:
            msg = "User {} in the system has a different passport login already".format(args["login"])
            current_app.logger.warn(msg)
            abort(make_response(jsonify(dict(code=1, fatal="User in the system has a different passport login already")), 200))

    # проверим роли пользователя на ноде
    same_node_permission = Permission.query.filter_by(uid=uid, node=node).with_for_update(True).first()
    if same_node_permission:
        if same_node_permission.role == role:
            db.session.rollback()
            msg = "User already has this role: {} - {} - {}".format(passportlogin, node, role)
            current_app.logger.warn(msg)
            return jsonify(dict(code=0, warning="User already has this role", data={"passport-login": passportlogin})), 200
        same_node_permission.role = role
        db.session.commit()
        current_app.logger.info("Grant role {role} for user {uid} on node {node}".format(role=role, node=node, uid=uid),
                                extra=dict(role=role, node=node, uid=uid))
        return jsonify(dict(code=0, data={"passport-login": passportlogin})), 200

    user = UserLogins.query.filter_by(uid=uid).with_for_update(True).first()
    if not user:
        user = UserLogins(uid=uid, passportlogin=passportlogin, internallogin=args["login"])
    new_permissions = Permission(role=role, node=node, uid=uid)
    user.permissions.append(new_permissions)

    db.session.add(user)

    log_action(AuditAction.AUTH_GRANT,
               user_id=None,
               node=node,
               grant_to=uid,
               role=role)
    db.session.commit()
    g.permissions = None
    current_app.logger.info("Grant role {role} for user {login} on node {node}".format(role=role, node=node, login=passportlogin),
                            extra=dict(role=role, node=node, uid=uid))
    return jsonify(dict(code=0, data={"passport-login": passportlogin})), 200


@idm_api.route('/idm/remove-role/', methods=['POST'])
def remove_role():
    _ensure_idm_tvm_ticket_is_ok()

    args = Schema({Required("login"): basestring,
                   Required("role"): dict,
                   Required("path"): basestring,
                   Required("fields"): dict,
                   Optional("fired", default=0): Coerce(int)})(get_data(request.form))

    uid, passportlogin, role, node = parse_args(args)

    if not uid:
        user = UserLogins.query.filter_by(passportlogin=passportlogin).with_for_update(True).first()
        if user:
            uid = UserLogins.uid
    else:
        user = UserLogins.query.filter_by(uid=uid).with_for_update(True).first()

    if not user:
        db.session.rollback()
        msg = "Employee no longer has this role: {} - {} - {}".format(passportlogin, node, role)
        current_app.logger.warn(msg)
        return jsonify(dict(code=0, warning="Employee no longer has this role")), 200

    # Если сотрудник уволен, отзовем все его роли
    all_user_permissions = Permission.query.filter_by(uid=uid).with_for_update(True).all()
    if args["fired"] == 1:
        for p in all_user_permissions:
            db.session.delete(p)
        db.session.delete(user)
        db.session.commit()
        msg = "Remove all role for user: {}".format(passportlogin)
        current_app.logger.warn(msg)
        return jsonify(dict(code=0)), 200
    # Также нужно удалить информацию о пользователе, если удаляемая роль единственная
    if len(all_user_permissions) == 1:
        # Проверим, что удаляем правильную роль
        permission = all_user_permissions[0]
        if permission.node == node and permission.role == role:
            db.session.delete(permission)
            db.session.delete(user)
            db.session.commit()
            msg = "Remove role for user: {} - {} - {}".format(passportlogin, node, role)
            current_app.logger.warn(msg)
            return jsonify(dict(code=0)), 200

    node_permission = None
    for p in all_user_permissions:
        if p.role == role and p.node == node:
            node_permission = p
    if not node_permission:
        db.session.rollback()
        msg = "Employee no longer has this role: {} - {} - {}".format(passportlogin, node, role)
        current_app.logger.warn(msg)
        return jsonify(dict(code=0, warning="Employee no longer has this role")), 200

    db.session.delete(node_permission)
    db.session.commit()
    msg = "Remove role for user: {} - {} - {}".format(passportlogin, node, role)
    current_app.logger.warn(msg)
    return jsonify(dict(code=0)), 200


@idm_api.route('/idm/info/')
def info():
    _ensure_idm_tvm_ticket_is_ok()
    services = [(s.id, s.name) for s in Service.query.order_by(Service.name).all()]
    fields = [dict(slug="passport-login", name=dict(en="Passport login", ru=u"Паспортный логин"), type="passportlogin", required=True)]
    roles = {s[0]: dict(name=s[1], roles=dict(slug="role", name={"en": "Choose role", "ru": u"Выберите роль"}, values=IDM_ROLES)) for s in services}
    main_services = {"all": dict(name=dict(en="General", ru=u"Общие"),
                                 help=dict(en="Global roles", ru=u"Глобальные роли"),
                                 roles=dict(slug="role", name={"en": "Choose role", "ru": u"Выберите роль"}, values=IDM_ROLES)),
                     "other": dict(name=dict(en="Service", ru=u"Сервис"),
                                   help=dict(en="Roles on service", ru=u"Роли по сервисам"),
                                   roles=dict(slug="service", name={"en": "Choose service", "ru": u"Выберите сервис"}, values=roles))}
    result = dict(code=0, roles=dict(slug="project", name={"en": "Choose type of role", "ru": u"Выберите тип роли"}, values=main_services),
                  fields=fields)
    return jsonify(result), 200


@idm_api.route('/idm/get-all-roles/')
def all_roles():
    _ensure_idm_tvm_ticket_is_ok()
    logins = [l.as_dict(with_permissions=True) for l in UserLogins.query.all()]
    users = {}
    for login in logins:
        for permission in login["permissions"]:
            role = permission["role"]
            if role in {"guest", "external_login"}:
                continue
            node = permission["node"].split("#")[-1]
            if node == "*":
                users[login["internallogin"]] = users.get(login["internallogin"], []) + [[{"project": "all", "role": role}, {"passport-login": login["passportlogin"]}]]
            else:
                users[login["internallogin"]] = users.get(login["internallogin"], []) + [[{"project": "other", "service": node, "role": role}, {"passport-login": login["passportlogin"]}]]
    user_roles = []
    for user, role in users.iteritems():
        user_role = dict(login=user, roles=role)
        user_roles.append(user_role)
    result = dict(code=0, users=user_roles)
    return jsonify(result), 200
