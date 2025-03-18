# coding=utf-8
"""
API for the admin-ui to manipulate configs

Crude solution on database
"""
import random
import re
import time
from copy import deepcopy
from datetime import datetime
from collections import defaultdict
from json.encoder import encode_basestring

import flask
from flask import Blueprint, jsonify, request, g, current_app, abort, make_response
from sqlalchemy.orm import joinedload, contains_eager, load_only
from voluptuous import Schema, Optional, Required, Coerce, All, Length, Boolean, Match, Invalid, Any, Unique
from voluptuous.error import MatchInvalid
from sqlalchemy.exc import IntegrityError
from sqlalchemy.dialects.postgresql import TEXT
from sqlalchemy import and_, text

from antiadblock.configs_api.lib.audit.audit import log_action
from antiadblock.configs_api.lib.auth.permissions import get_user_permissions, ensure_permissions_on_service, \
    ensure_permissions_on_domain, ensure_global_permissions, check_permissions_user_or_service
from antiadblock.configs_api.lib.auth.auth import ensure_tvm_ticket_is_ok
from antiadblock.configs_api.lib.auth.permission_models import PermissionKind
from antiadblock.configs_api.lib.context import CONTEXT
from antiadblock.configs_api.lib.validation.template import TEMPLATE

from antiadblock.configs_api.lib.validation.validation_utils import Every, prepend_invalid_exception_path
from antiadblock.configs_api.lib.validation.complex_validation import check_tokens_on_create, check_tokens_on_activate, run_static_tests, check_experiment_data
from antiadblock.configs_api.lib.creation.creation import generate_start_config, generate_token as make_token
from antiadblock.configs_api.lib.utils import check_args, cryprox_decrypt_request
from antiadblock.configs_api.lib.jsonlogging import logging_context
from antiadblock.configs_api.lib.dashboard.dashboard_api import get_actual_dashboard_checks_id
from antiadblock.configs_api.lib.db import Service, Config, db, ConfigMark, ServiceComments, ServiceChecks, ChecksInProgress
from antiadblock.configs_api.lib.db_utils import get_config_by_status, is_service_active, \
    get_service_id_for_label, is_label_exist, get_data_from_parent_configs, get_final_configs, merge_data, \
    get_exp_ids_from_parent_labels, create_lock
from antiadblock.configs_api.lib.models import ConfigStatus, ServiceStatus, AuditAction, TrendType, TrendDeviceType, ServiceSupportPriority
from antiadblock.configs_api.lib.const import DECRYPT_RESPONSE_HEADER_NAME, NONE_DECRYPT_URL_ARG_MSG_TEMPLATE, \
    FAILED_TO_DECRYPT_MSG, FORBIDDEN_URL_DECRYPT_MSG, EMPTY_URL_DECRYPT_MSG, SERVICE_IS_INACTIVE_MSG, ROOT_LABEL_ID, \
    ANTIADB_SUPPORT_QUEUE, READY_FOR_FILL_TAG

api = Blueprint('api', __name__)


TICKET_TITLES_MAP = {
    TrendType.NEGATIVE_TREND: u'Негативный тренд на {}',
    TrendType.MONEY_DROP: u'Падение денег на {}'
}


SUPPORT_PRIORITY_MAP = {
    TrendType.NEGATIVE_TREND: {
        ServiceSupportPriority.CRITICAL: 'critical',
        ServiceSupportPriority.MAJOR: 'critical',
        ServiceSupportPriority.MINOR: 'normal',
        ServiceSupportPriority.OTHER: 'minor'
    },
    TrendType.MONEY_DROP: {
        ServiceSupportPriority.CRITICAL: 'blocker',
        ServiceSupportPriority.MAJOR: 'critical',
        ServiceSupportPriority.MINOR: 'normal',
        ServiceSupportPriority.OTHER: 'normal'
    }
}


@api.route('/services')
def get_all_services():
    # TODO: add services allowed by web master
    user_id = CONTEXT.auth.get_user_id()
    service_ids = get_user_permissions(user_id).get_permitted_services()
    result = Service.query.filter(Service.id.in_(service_ids)).order_by(Service.name).all()
    return jsonify(dict(items=[service.as_dict() for service in result], total=len(result)))


@api.route('/service/<string:service_id>')
def get_service(service_id):
    ensure_permissions_on_service(service_id, PermissionKind.SERVICE_SEE)
    service = Service.query.get_or_404(service_id)
    return jsonify(service.as_dict())


@api.route('/labels')
def get_labels():
    args = check_args({Required("service_id"): basestring})
    service_id = args.get("service_id")
    ensure_permissions_on_service(service_id, PermissionKind.SERVICE_SEE)

    if not is_service_active(service_id):
        abort(make_response(jsonify(dict(message=SERVICE_IS_INACTIVE_MSG)), 400))

    # делаем рекурсивный запрос в БД, чтобы получить все дерево иерархии
    # не уверен, что такое можно сделать с помощью Sqlalchemy
    raw_query = text("""
        WITH RECURSIVE r AS (
            SELECT NULL::VARCHAR as parent_label_id, '{}'::VARCHAR as label_id
            UNION ALL
            SELECT DISTINCT configs.parent_label_id, configs.label_id
            FROM configs
                JOIN r
                    ON configs.parent_label_id = r.label_id
            WHERE configs.service_id = '{}' or configs.service_id is NULL
        )
        SELECT * FROM r;
        """.format(ROOT_LABEL_ID, service_id))
    results = db.engine.execute(raw_query)

    tmp_tree = defaultdict(dict)

    for r in results:
        if r.parent_label_id is None:
            continue
        tmp_tree[r.parent_label_id][r.label_id] = {}

    for p_id, v_id in tmp_tree.iteritems():
        for k in v_id.keys():
            if k in tmp_tree:
                v_id[k] = tmp_tree[k]

    return jsonify({ROOT_LABEL_ID: deepcopy(tmp_tree.get(ROOT_LABEL_ID, {}))})


@api.route('/label/<string:label_id>/configs')
def get_label_configs(label_id):
    service_id = get_service_id_for_label(label_id)
    ensure_permissions_on_service(service_id, PermissionKind.SERVICE_SEE)
    args = check_args({Optional("offset", default=0): Coerce(int),
                       Optional("limit", default=20): Coerce(int),
                       Optional("show_archived", default=False): Boolean()})
    query = Config.query.filter_by(label_id=label_id)
    if not args["show_archived"]:
        query = query.filter_by(archived=False)
    overall = query.count()
    items = query.order_by(Config.created.desc()).limit(args["limit"]).offset(args["offset"]).options(joinedload(Config.statuses))

    return jsonify(dict(items=[_filter_hidden_fields(item.as_dict()) for item in items],
                        total=overall))


def _get_config_json_by_status(service_id, label_id, status, user_id):
    config = get_config_by_status(service_id=service_id, label_id=label_id, status=status)
    config_dict = config.as_dict()
    # если пришел пользователь, то нужно пофильтровать поля в соответствии с его правами
    if user_id:
        config_dict["data"] = TEMPLATE.filter_hidden_fields(service_id=service_id, config_data=config_dict["data"])
    return jsonify(config_dict)


@api.route('/label/<string:label_id>/config/active')
def get_active_config(label_id):
    service_id = get_service_id_for_label(label_id)
    user_id = check_permissions_user_or_service(service_id, PermissionKind.SERVICE_SEE)
    return _get_config_json_by_status(service_id=service_id, label_id=label_id, status=ConfigStatus.ACTIVE, user_id=user_id)


@api.route('/label/<string:label_id>/config/test')
def get_test_config(label_id):
    service_id = get_service_id_for_label(label_id)
    user_id = check_permissions_user_or_service(service_id, PermissionKind.SERVICE_SEE)
    return _get_config_json_by_status(service_id=service_id, label_id=label_id, status=ConfigStatus.TEST, user_id=user_id)


@api.route('/label/<string:label_id>/config/schema')
def get_config_schema(label_id):
    service_id = get_service_id_for_label(label_id)
    ensure_permissions_on_service(service_id, PermissionKind.SERVICE_SEE)
    schema = TEMPLATE.get_frontend_schema(service_id)
    return jsonify(schema)


@api.route('/config/<int:config_id>')
def get_label_config(config_id):
    config = Config.query.options(joinedload(Config.statuses)).filter_by(id=config_id).first()
    if config is None:
        # return 403 if user don't have permission
        ensure_global_permissions(PermissionKind.SERVICE_SEE)
        # else return 404
        return jsonify(dict(message="Config no found")), 404
    service_id = config.service_id
    label_id = config.label_id
    ensure_permissions_on_service(service_id, PermissionKind.SERVICE_SEE)

    args = check_args({Optional("status"): Any(ConfigStatus.ACTIVE, ConfigStatus.TEST),
                       Optional("exp_id"): basestring})

    status = args.get("status", ConfigStatus.ACTIVE)
    exp_id = args.get("exp_id")

    config_dict = config.as_dict()
    config_dict["data"] = TEMPLATE.filter_hidden_fields(service_id=service_id, config_data=config_dict["data"])
    # get data from parents_config
    parent_label_id = config.parent_label_id

    config_dict["parent_data"], _ = get_data_from_parent_configs(label_id, parent_label_id, None, status, exp_id, with_defaults=True)
    return jsonify(config_dict)


def basestring_strip(value):
    if isinstance(value, str):
        return value.strip()
    return value.encode('utf-8').strip().decode('utf-8')


def domain_exists(domain):
    return bool(Service.query.filter_by(domain=domain).first())


def service_id_exists(service_id):
    return bool(Service.query.filter_by(id=service_id).first())


def validate_parent_label_id(parent_label_id):
    if not is_label_exist(parent_label_id):
        raise Invalid(u"Некорректное значение для родительского конфига")
    return parent_label_id


@api.route('/service', methods=['POST'])
def create_service():
    """При создании сервиса создается начальный конфиг, необходимо указать родительский конфиг"""
    ensure_global_permissions(PermissionKind.SERVICE_CREATE)

    def _normalize_domain(domain):
        try:
            lower = domain.lower().strip()
            if len(lower) > 4 and lower[:4] == "www.":
                lower = lower[4:]
            return lower.encode("idna")
        except Exception:
            raise Invalid(u"Введен невалидный домен")

    def _validate_domain(domain):
        if domain_exists(domain):
            raise Invalid(u"Данный домен уже используется")
        return domain

    def _match(domain):
        try:
            Match(r"^[\w.\-]*$")
            return domain
        except MatchInvalid:
            raise Invalid(u"Введен невалидный домен")

    def _validate_service_id(service_id):
        if service_id_exists(service_id):
            raise Invalid(u"Сервис с таким service_id уже существует")
        if re.match(r"^[a-z0-9.]+$", service_id) is None:
            raise Invalid(u"Недопустимое имя service_id.\n"
                          u" Разрешены только буквы латинского алфавита в нижнем регистре, цифры и точки")
        return service_id

    args = Schema({Required("service_id"): All(basestring,
                                               basestring_strip,
                                               _validate_service_id,
                                               Length(min=1, msg=u"ID сервиса не может быть пустым"),
                                               Length(min=3, msg=u"ID сервиса не может содержать менее 3 символов")),
                   Required("name"): All(basestring,
                                         basestring_strip,
                                         Length(min=1, msg=u"Название сервиса не может быть пустым"),
                                         Length(min=3, msg=u"Название сервиса не может содержать менее 3 символов")),
                   Required("domain"): All(basestring,
                                           basestring_strip,
                                           Length(min=1, msg=u"Домен не может быть пустым"),
                                           Length(min=2, msg=u"Домен не может содержать менее 2 символов"),
                                           _match,
                                           _normalize_domain,
                                           _validate_domain),
                   Optional("parent_label_id"): All(basestring,
                                                    validate_parent_label_id),
                   Optional("support_priority"): Any(*ServiceSupportPriority.all())})(request.get_json(force=True))

    user_id = ensure_permissions_on_domain(args["domain"])

    # TODO: think about more safety scheme to do it
    seed = request.headers.get("X-GENERATION-SEED")
    if seed:
        random.seed(seed)

    # TODO: think about id collision case
    service = Service(id=args['service_id'], name=args["name"], status=ServiceStatus.OK, owner_id=user_id,
                      domain=args['domain'],
                      support_priority=args.get("support_priority", ServiceSupportPriority.OTHER))

    log_action(action=AuditAction.SERVICE_CREATE,
               user_id=user_id,
               service_id=args["service_id"],
               service_name=args["name"],
               service_domain=args['domain'],
               label_id=args["service_id"])

    config_data = generate_start_config(service_id=service.id, domain=args['domain'])
    # TODO: localize initial config name?
    config = Config(comment=u'Начальные настройки',
                    data=config_data,
                    created=datetime.utcnow(),
                    creator_id=user_id,
                    label_id=service.id,
                    parent_label_id=args.get("parent_label_id", ROOT_LABEL_ID),
                    )
    config.statuses.append(ConfigMark(status=ConfigStatus.ACTIVE))
    config.statuses.append(ConfigMark(status=ConfigStatus.TEST))
    config.statuses.append(ConfigMark(status=ConfigStatus.APPROVED))
    service.configs.append(config)

    db.session.add(service)
    db.session.commit()
    flask.current_app.logger.info("Service created", extra=dict(service=service.as_dict(),
                                                                tokens=config_data["PARTNER_TOKENS"],
                                                                user_id=user_id))
    return jsonify(service.as_dict()), 201


@api.route('/label', methods=['POST'])
def create_label():
    user_id = ensure_global_permissions(PermissionKind.LABEL_CREATE)
    args = Schema({Required("label_id"): All(basestring),
                   Required("parent_label_id"): All(basestring,
                                                    validate_parent_label_id)})(request.get_json(force=True))
    parent_label_id = args["parent_label_id"]
    label_id = args["label_id"]
    if is_label_exist(label_id):
        return jsonify({"message": u"Label: {} уже существует".format(label_id)}), 400

    # нужно забрать service_id из родительского конфига
    service_id = get_service_id_for_label(parent_label_id)

    config = Config(
        comment="Initial empty config",
        service_id=service_id,
        data={},
        created=datetime.utcnow(),
        creator_id=user_id,
        label_id=label_id,
        parent_label_id=parent_label_id,
    )
    config.statuses.append(ConfigMark(status=ConfigStatus.ACTIVE))
    config.statuses.append(ConfigMark(status=ConfigStatus.TEST))
    config.statuses.append(ConfigMark(status=ConfigStatus.APPROVED))
    db.session.add(config)
    try:
        log_action(action=AuditAction.LABEL_CREATE,
                   service_id=service_id,
                   user_id=user_id,
                   label_id=label_id,
                   parent_label_id=parent_label_id,
                   )
        db.session.commit()
    except IntegrityError as ex:
        msg = u"Обнаружены параллельные изменения конфигов"
        flask.current_app.logger.info(msg, extra=dict(label_id=label_id,
                                                      user_id=user_id))
        flask.current_app.logger.error(ex)
        return jsonify(dict(message=msg)), 409

    flask.current_app.logger.info("Label created", extra=dict(label_id=label_id,
                                                              config_id=config.id,
                                                              user_id=user_id))

    return jsonify(dict(items=[_filter_hidden_fields(config.as_dict(with_statuses=True))], total=1)), 201


@api.route('/label/<string:label_id>/change_parent_label', methods=['POST'])
def change_parent_label(label_id):
    user_id = ensure_global_permissions(PermissionKind.CHANGE_PARENT_LABEL)
    service_id = get_service_id_for_label(label_id)
    # do not change ROOT parent
    if label_id == ROOT_LABEL_ID:
        abort(make_response(jsonify(dict(message="Can't change parent for {}".format(ROOT_LABEL_ID))), 400))
    args = Schema({Required("parent_label_id"): All(basestring,
                                                    validate_parent_label_id)})(request.get_json(force=True))
    parent_label_id = args["parent_label_id"]

    # check loop
    l_id = parent_label_id
    label_ids = {l_id}
    while l_id != ROOT_LABEL_ID:
        config = Config.query.filter_by(label_id=l_id).first()
        l_id = config.parent_label_id
        label_ids.add(l_id)
        if label_id in label_ids:
            abort(make_response(jsonify(dict(message="Can't change parent for {}".format(label_id))), 400))

    for config in Config.query.filter_by(label_id=label_id):
        old_parent_label_id = config.parent_label_id
        config.parent_label_id = parent_label_id

    try:
        log_action(action=AuditAction.CHANGE_PARENT_LABEL,
                   service_id=service_id,
                   user_id=user_id,
                   label_id=label_id,
                   old_parent_label_id=old_parent_label_id,
                   parent_label_id=parent_label_id,
                   )
        db.session.commit()
    except IntegrityError as ex:
        msg = u"Обнаружены параллельные изменения конфигов"
        flask.current_app.logger.info(msg, extra=dict(service_id=service_id,
                                                      user_id=user_id))
        flask.current_app.logger.error(ex)
        return jsonify(dict(message=msg)), 409

    return "{}", 201


@api.route('/label/<string:label_id>/config', methods=['POST'])
def create_label_config(label_id):

    def check_current_cookie(data):
        current_cookie = data.get('CURRENT_COOKIE', '')
        if current_cookie:
            # необходима проверка, что значение CURRENT_COOKIE присутствует в списке разрешенных кук WHITELIST_COOKIES
            if current_cookie not in data.get('WHITELIST_COOKIES', []):
                raise Invalid(u"Значение 'CURRENT_COOKIE' должно присутствовать в 'WHITELIST_COOKIES'", path=["CURRENT_COOKIE"])
            # И отсутствует в списке предыдущих кук DEPRECATED_COOKIES
            if current_cookie in data.get('DEPRECATED_COOKIES', []):
                raise Invalid(u"Значение 'CURRENT_COOKIE' должно отсутствовать в 'DEPRECATED_COOKIES'", path=["CURRENT_COOKIE"])
        return data

    service_id = get_service_id_for_label(label_id)

    if request.headers.get("X-Ya-Service-Ticket"):
        ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=["AAB_SANDBOX_MONITORING_TVM_ID", "AAB_CRYPROX_TVM_CLIENT_ID"])
        user_id = 0
    else:
        if service_id:
            user_id = ensure_permissions_on_service(service_id, PermissionKind.CONFIG_CREATE)
            if not is_service_active(service_id):
                abort(make_response(jsonify(dict(message=SERVICE_IS_INACTIVE_MSG)), 400))
        else:
            user_id = ensure_global_permissions(PermissionKind.PARENT_CONFIG_CREATE)
    args = Schema({Required("comment"): All(basestring,
                                            Length(min=1, msg=u"Комментарий должен содержать как минимум 1 символ"),
                                            Length(max=60, msg=u"Длина комментария не должна превышать 60 символов")),
                   Required("data"): All(dict, check_current_cookie),
                   # data_settings = {'key': {'UNSET': True}, ...}
                   Required("data_settings"): dict,
                   Required("parent_id"): int})(request.get_json(force=True))

    parent_id = args["parent_id"]
    parent = Config.query.filter_by(label_id=label_id, id=parent_id).first_or_404()
    parent_data = parent.data
    deleted_keys = TEMPLATE.filter_hidden_fields(service_id=None, config_data=parent_data).viewkeys() - args["data"].viewkeys()

    merged_config = deepcopy(parent.data)
    for k in deleted_keys:
        del merged_config[k]
    for k, v in args['data'].iteritems():
        merged_config[k] = v

    config = Config(
        comment=args['comment'],
        data=merged_config,
        data_settings=args['data_settings'],
        created=datetime.utcnow(),
        service_id=parent.service_id,
        creator_id=user_id,
        parent_id=parent_id,
        label_id=label_id,
        parent_label_id=parent.parent_label_id,
    )

    @prepend_invalid_exception_path(prepend_path=Config.data.name)
    def validate():
        if user_id:
            Schema(Every(check_experiment_data(),
                         TEMPLATE.get_type_validation_schema_without_required(),
                         TEMPLATE.get_immutable_fields_validation_schema(service_id=service_id, parent_config_data=parent_data)))(config.data)
        else:
            Schema(Every(check_experiment_data(),
                         TEMPLATE.get_type_validation_schema_without_required()))(config.data)

    validate()
    db.session.add(config)

    try:
        db.session.commit()
    except IntegrityError as ex:
        msg = u"Обнаружены параллельные изменения конфигов"
        flask.current_app.logger.info(msg, extra=dict(label_id=label_id,
                                                      user_id=user_id))
        flask.current_app.logger.error(ex)
        return jsonify(dict(message=msg)), 409

    flask.current_app.logger.info("Config created", extra=dict(label_id=label_id,
                                                               config_id=config.id,
                                                               user_id=user_id))

    return jsonify(config.as_dict(with_statuses=True)), 201


@api.route('/config/<int:config_id>', methods=['PATCH'])
def archive_config(config_id):
    config = Config.query.options(load_only("service_id")).filter_by(id=config_id).first()
    if not config:
        abort(404)
    service_id = config.service_id
    user_id = ensure_permissions_on_service(service_id, PermissionKind.CONFIG_ARCHIVE)
    args = Schema({Required("archived"): bool})(request.get_json(force=True))
    with logging_context(service_id=None, config_id=config_id, user_id=user_id):
        query = Config.query.outerjoin(ConfigMark).options(contains_eager(Config.statuses)).filter(Config.id == config_id)
        if service_id:
            if not is_service_active(service_id):
                abort(make_response(jsonify(dict(message=SERVICE_IS_INACTIVE_MSG)), 400))
            query = query.filter(Config.service_id == service_id)
        configs = query.with_for_update(of=Config).all()

        if not configs:
            abort(404)

        config = configs[0]
        config_statuses = []
        for cfg in configs:
            config_statuses.extend(cfg.get_statuses())
        if ConfigStatus.TEST in config_statuses or ConfigStatus.ACTIVE in config_statuses:
            msg = "Archiving config in status 'active' or 'test' is forbidden"
            flask.current_app.logger.warn(msg)
            return jsonify(dict(message=u"Архивация тестовых и активных конфигов запрещена")), 400

        config.archived = args['archived']
        log_action(action=AuditAction.CONFIG_ARCHIVE, service_id=config.service_id, user_id=user_id,
                   config_id=config_id, archived=args['archived'], label_id=config.label_id)
        db.session.commit()
        flask.current_app.logger.info("Config {} was archived".format(config_id))
    return jsonify(config.as_dict()), 200


def validate_final_configs(config, old_config=None, status=ConfigStatus.ACTIVE, exp_id=None):
    # validate all final configs
    final_configs = get_final_configs(config.label_id, status=status, exp_id=exp_id)

    @prepend_invalid_exception_path(prepend_path=Config.data.name)
    def validate(service_id, config_data):
        Schema(Every(TEMPLATE.get_type_validation_schema(),
                     run_static_tests,
                     check_tokens_on_create(service_id)))(config_data)

    @prepend_invalid_exception_path(prepend_path=Config.data.name)
    def validate_tokens(old_merge_config, new_merge_config):
        Schema(check_tokens_on_activate(old_merge_config))(new_merge_config)

    validate_with_final_old_config = False
    if not final_configs["configs"]:
        final_configs["configs"][config.service_id] = config
        validate_with_final_old_config = True

    for f_config in final_configs["configs"].values():
        old_parent_data, new_parent_data = get_data_from_parent_configs(f_config.label_id, f_config.parent_label_id, config, status=status, exp_id=exp_id)
        if validate_with_final_old_config and old_config is not None:
            old_merge_config = TEMPLATE.fill_defaults(merge_data(old_parent_data, old_config.data, old_config.data_settings))
        else:
            old_merge_config = TEMPLATE.fill_defaults(merge_data(old_parent_data, f_config.data, f_config.data_settings))
        new_merge_config = TEMPLATE.fill_defaults(merge_data(new_parent_data, f_config.data, f_config.data_settings))
        validate(f_config.service_id, new_merge_config)
        if status == ConfigStatus.ACTIVE:
            validate_tokens(old_merge_config, new_merge_config)


@api.route('/config/<int:config_id>/experiment', methods=['PATCH'])
def experiment_config(config_id):
    configs = Config.query.outerjoin(ConfigMark).options(contains_eager(Config.statuses)).filter(Config.id == config_id).with_for_update(of=Config).all()
    if configs is None:
        # return 403 if user don't have permission
        ensure_global_permissions(PermissionKind.CONFIG_MARK_EXPERIMENT)
        # else return 404
        return jsonify(dict(message="Config no found")), 404

    config = configs[0]
    config_statuses = []
    for cfg in configs:
        config_statuses.extend(cfg.get_statuses())

    service_id = config.service_id
    user_id = ensure_permissions_on_service(service_id, PermissionKind.CONFIG_MARK_EXPERIMENT)
    args = Schema({Required("exp_id"): basestring})(request.get_json(force=True))
    exp_id = args["exp_id"]

    with logging_context(service_id=service_id, config_id=config_id, user_id=user_id):

        if config.archived:
            db.session.rollback()
            msg = "Couldn't mark for experiment archived config"
            flask.current_app.logger.warn(msg)
            return jsonify(dict(message=msg)), 400

        for status in (ConfigStatus.DECLINED, ConfigStatus.TEST, ConfigStatus.ACTIVE):
            if status in config_statuses:
                db.session.rollback()
                msg = "Couldn't mark for experiment {} config".format(status)
                flask.current_app.logger.warn(msg)
                return jsonify(dict(message=msg)), 400

        # validate all final configs
        validate_final_configs(config, exp_id=exp_id)

        # trying get previous config with same exp_id
        old_config = Config.query.filter_by(label_id=Config.label_id, exp_id=exp_id).with_for_update(of=Config).first()
        if old_config:
            old_config.exp_id = None

        config.exp_id = exp_id
        log_action(action=AuditAction.CONFIG_EXPERIMENT, service_id=config.service_id, user_id=user_id,
                   config_id=config_id, exp_id=exp_id, label_id=config.label_id)
        db.session.commit()
        flask.current_app.logger.info("Config {} marked for exp_id={}".format(config_id, exp_id))

    return jsonify(config.as_dict()), 200


@api.route('/config/<int:config_id>/experiment/remove', methods=['PATCH'])
def remove_experiment_from_config(config_id):
    config = Config.query.filter_by(id=config_id).with_for_update(of=Config).first()
    if config is None:
        # return 403 if user don't have permission
        ensure_global_permissions(PermissionKind.CONFIG_MARK_EXPERIMENT)
        # else return 404
        return jsonify(dict(message="Config no found")), 404
    service_id = config.service_id
    user_id = ensure_permissions_on_service(service_id, PermissionKind.CONFIG_MARK_EXPERIMENT)

    if not config.exp_id:
        return jsonify(dict(message="Config without exp_id")), 400

    # validate all final configs
    validate_final_configs(config, exp_id=config.exp_id)

    config.exp_id = None
    log_action(action=AuditAction.CONFIG_EXPERIMENT, service_id=config.service_id, user_id=user_id, config_id=config_id,
               exp_id=None, label_id=config.label_id)
    db.session.commit()
    flask.current_app.logger.info("Experiment removed from config {}".format(config_id))

    return jsonify(config.as_dict()), 200


@api.route('/label/<string:label_id>/experiment')
def get_experiment_ids_for_label(label_id):
    """
    :param label_id:
    :return: list of existing experiment_ids
    """
    service_id = get_service_id_for_label(label_id)
    ensure_permissions_on_service(service_id, PermissionKind.SERVICE_SEE)
    config = Config.query.options(load_only("label_id")).filter_by(label_id=label_id).first()

    if not config:
        return jsonify({"items": list(), "total": 0}), 200
    results = get_exp_ids_from_parent_labels(label_id, config.parent_label_id)
    return jsonify({"items": results, "total": len(results)}), 200


def _retrieve_configs_for_update(service_id, old_config_id, new_config_id):
    """
    Selects and locks config that take part in activating config: new and old one. Perform one operation locking to
    prevent deadlocks.
    :param service_id:
    :param old_config_id:
    :param new_config_id:
    :return old_config, new config:
    """
    def first_or_none(elements):
        return None if len(elements) == 0 else elements[0]

    # FIXME: filter only by target status when multistatus configs will be available
    query = Config.query.outerjoin(ConfigMark).options(contains_eager(Config.statuses)).filter(Config.id.in_((old_config_id, new_config_id)))
    if service_id:
        query = query.filter(Config.service_id == service_id)
    query = query.with_for_update(of=Config).all()
    configs = [config for config in query]
    new_config = first_or_none(filter(lambda c: c.id == new_config_id, configs))
    old_config = first_or_none(filter(lambda c: c.id == old_config_id, configs))

    return old_config, new_config


@api.route('/label/<string:label_id>/config/<int:config_id>/active', methods=['PUT'])
def activate_config(config_id, label_id):
    """
      Move config to 'active' status. Old test config move to 'inactive' status
    """
    service_id = get_service_id_for_label(label_id)
    user_id = check_permissions_user_or_service(service_id, PermissionKind.CONFIG_MARK_ACTIVE)

    old_config_id = Schema({Required("old_id"): int})(request.get_json(force=True))["old_id"]
    old_config, new_config = _retrieve_configs_for_update(service_id, old_config_id, config_id)
    with logging_context(service_id=service_id, config_id=config_id, old_config_id=old_config_id, user_id=user_id):
        if not old_config or not new_config:
            flask.current_app.logger.info("Config activation failed: config or previous config not found by id")
            return "{}", 404

        if old_config.id == config_id:
            return "{}", 400

        if ConfigStatus.ACTIVE not in old_config.get_statuses():
            db.session.rollback()
            msg = "Conflict changes detected. Old config has marks {}".format(old_config.get_statuses())
            flask.current_app.logger.warn(msg)
            return jsonify(dict(message=u"Кто-то недавно изменил конфиги. Попробуйте обновить страницу")), 409

        if new_config.archived:
            db.session.rollback()
            msg = "Couldn't activate archived config"
            flask.current_app.logger.warn(msg)
            return jsonify(dict(message=u"Невозможно активировать заархивированный конфиг")), 400

        if new_config.exp_id:
            db.session.rollback()
            msg = "Couldn't activate experiment config"
            flask.current_app.logger.warn(msg)
            return jsonify(dict(message=u"Невозможно активировать экспериментальный конфиг")), 400

        if ConfigStatus.DECLINED in new_config.get_statuses():
            db.session.rollback()
            msg = "Couldn't activate declined config"
            flask.current_app.logger.warn(msg)
            return jsonify(dict(message=u"Невозможно активировать отклоненный конфиг")), 400

        if ConfigStatus.APPROVED not in new_config.get_statuses():
            if user_id:
                ensure_permissions_on_service(service_id, PermissionKind.CONFIG_MODERATE)
            new_config.statuses.append(ConfigMark(status=ConfigStatus.APPROVED))

        if service_id and not is_service_active(service_id):
            abort(make_response(jsonify(dict(message=SERVICE_IS_INACTIVE_MSG)), 400))

        # validate all final configs
        validate_final_configs(new_config, old_config=old_config, status=ConfigStatus.ACTIVE)

        status_to_remove = filter(lambda s: s.status == ConfigStatus.ACTIVE, old_config.statuses)[0]
        db.session.delete(status_to_remove)
        new_config.statuses.append(ConfigMark(status=ConfigStatus.ACTIVE))

        log_action(action=AuditAction.CONFIG_ACTIVE,
                   service_id=service_id,
                   user_id=user_id,
                   config_id=config_id,
                   label_id=label_id,
                   old_config_id=old_config_id,
                   config_comment=new_config.comment)

        db.session.commit()
        flask.current_app.logger.info("Config marked 'active'")

    if service_id:
        try:
            CONTEXT.charts_client.push_comment(user_id, service_id, config_id, old_config_id, CONTEXT.blackbox)
        except Exception as e:
            flask.current_app.logger.error("Failed publish comment. Error is {}".format(e.message))

        try:
            CONTEXT.infra_handler.send_event_config_updated(service_id=service_id,
                                                            label_id=label_id,
                                                            old_config_id=old_config_id,
                                                            new_config_id=config_id,
                                                            config_comment=new_config.comment,
                                                            is_test_config=False,
                                                            device_type=new_config.data.get('DEVICE_TYPE'))
        except Exception as e:
            flask.current_app.logger.error("Infra handler has failed: {}".format(str(e)))

    return "{}", 200


@api.route('/label/<string:label_id>/config/<int:config_id>/test', methods=['PUT'])
def test_config(config_id, label_id):
    """
    Move config to 'test' status. Old 'test' config move to 'inactive' status. If there was no config in test status
    then old_id expected to be targeted to 'active' config. In this case 'active' config will not change its status
    """
    service_id = get_service_id_for_label(label_id)
    user_id = check_permissions_user_or_service(service_id, PermissionKind.CONFIG_MARK_TEST)

    old_config_id = Schema({Required("old_id"): int})(request.get_json(force=True))["old_id"]
    if old_config_id == config_id:
        return "{}", 400

    old_config, new_config = _retrieve_configs_for_update(service_id, old_config_id, config_id)
    with logging_context(service_id=service_id, config_id=config_id, old_config_id=old_config_id, user_id=user_id):
        if not new_config or not old_config:
            flask.current_app.logger.info("Move config to test failed. Config or previous config not found by id")
            return "{}", 404

        if ConfigStatus.TEST not in old_config.get_statuses():
            msg = "Mark config as a test failed. Old config has no mark {}".format(ConfigStatus.TEST)
            flask.current_app.logger.warn(msg)
            return jsonify(dict(message=u"Кто-то недавно изменил конфиги. Попробуйте обновить страницу")), 409

        if new_config.archived:
            db.session.rollback()
            msg = "Couldn't mark test archived config"
            flask.current_app.logger.warn(msg)
            return jsonify(dict(message=u"Невозможно поставить метку 'Тест' на заархивированный конфиг")), 400

        if new_config.exp_id:
            db.session.rollback()
            msg = "Couldn't mark test experiment config"
            flask.current_app.logger.warn(msg)
            return jsonify(dict(message=u"Невозможно поставить метку 'Тест' на экспериментальный конфиг")), 400

        if ConfigStatus.TEST in new_config.get_statuses():
            msg = "Mark config as a test failed. New config already has mark {}".format(ConfigStatus.TEST)
            flask.current_app.logger.warn(msg)
            return jsonify(dict(message=u"Невозможно поставить метку 'Тест'. Метка уже существует")), 400

        if ConfigStatus.DECLINED in new_config.get_statuses():
            db.session.rollback()
            msg = "Couldn't activate declined config"
            flask.current_app.logger.warn(msg)
            return jsonify(dict(message=u"Невозможно активировать отклоненный конфиг")), 400

        if service_id and not is_service_active(service_id):
            abort(make_response(jsonify(dict(message=SERVICE_IS_INACTIVE_MSG)), 400))

        # validate all final configs
        validate_final_configs(new_config, old_config=old_config, status=ConfigStatus.TEST)

        status_to_remove = filter(lambda s: s.status == ConfigStatus.TEST, old_config.statuses)[0]
        db.session.delete(status_to_remove)
        new_config.statuses.append(ConfigMark(status=ConfigStatus.TEST))

        log_action(action=AuditAction.CONFIG_TEST,
                   service_id=service_id,
                   user_id=user_id,
                   config_id=config_id,
                   old_config_id=old_config_id,
                   config_comment=new_config.comment,
                   label_id=label_id)
        db.session.commit()
        flask.current_app.logger.info("Config marked 'test'")

    if service_id:
        try:
            CONTEXT.infra_handler.send_event_config_updated(service_id=service_id,
                                                            label_id=label_id,
                                                            old_config_id=old_config_id,
                                                            new_config_id=config_id,
                                                            config_comment=new_config.comment,
                                                            is_test_config=True,
                                                            device_type=new_config.data.get('DEVICE_TYPE'))
        except Exception as e:
            flask.current_app.logger.error("Infra handler has failed: {}".format(str(e)))

    return "{}", 200


@api.route('/service/<string:service_id>/utils/decrypt_url', methods=['POST'])
def decrypt_url_handler(service_id):
    """
        Decrypt single url. Using in Adminka
    """
    ensure_permissions_on_service(service_id, PermissionKind.SERVICE_SEE)

    crypted_url = Schema({
        Required('url', msg=NONE_DECRYPT_URL_ARG_MSG_TEMPLATE):
            All(basestring,
                Length(min=1, msg=EMPTY_URL_DECRYPT_MSG)),
    })(request.get_json(force=True))["url"]

    if crypted_url is None:
        msg = NONE_DECRYPT_URL_ARG_MSG_TEMPLATE
        current_app.logger.error(msg)
        return jsonify({'message': msg, 'request_id': g.request_id}), 400

    response = cryprox_decrypt_request(service_id, crypted_url, CONTEXT.cryprox, current_app)

    decrypted_url = response.headers.get(DECRYPT_RESPONSE_HEADER_NAME)

    if decrypted_url == crypted_url or 300 < response.status_code < 400:
        current_app.logger.error(FAILED_TO_DECRYPT_MSG)
        return jsonify({'message': FAILED_TO_DECRYPT_MSG, 'request_id': g.request_id}), 404
    if response.status_code == 403:
        current_app.logger.error(FORBIDDEN_URL_DECRYPT_MSG)
        return jsonify({'message': FORBIDDEN_URL_DECRYPT_MSG, 'request_id': g.request_id}), 403
    if response.status_code != 200:
        current_app.logger.error('Failed to decrypt url `{crypted_url}` using {cryprox_host}. Reason: {response_code} {response_text}'
                                 .format(crypted_url=crypted_url,
                                         cryprox_host=CONTEXT.cryprox.host,
                                         response_code=response.status_code,
                                         response_text=response.text))
        return jsonify({'message': 'Failed to decrypt url due to internal error', 'request_id': g.request_id}), 500

    return jsonify({'decrypted_url': decrypted_url})


def _set_service_status(service_id, status):
    """
    Change service status: ServiceStatus.OK <-> ServiceStatus.INACTIVE
    """
    if status not in {ServiceStatus.INACTIVE, ServiceStatus.OK}:
        return "{}", 400
    user_id = ensure_permissions_on_service(service_id, PermissionKind.SERVICE_STATUS_SWITCH)
    with logging_context(service_id=service_id, user_id=user_id):
        service = Service.query.filter(Service.id == service_id).with_for_update(of=Service).first_or_404()
        if service.status not in {ServiceStatus.INACTIVE, ServiceStatus.OK}:
            msg = "Couldn't change service status. Current service status '{}' is wrong".format(service.status)
            flask.current_app.logger.info(msg)
            return jsonify(dict(message=u"Невозможно изменить статус. Текущий статус сервиса некорректный")), 400
        if service.status == status:
            msg = "Couldn't change service status. Status already set"
            flask.current_app.logger.info(msg)
            return jsonify(dict(message=u"Невозможно изменить статус. Он уже установлен")), 400
        service.status = status
        log_action(action=AuditAction.SERVICE_STATUS_SWITCH,
                   service_id=service_id,
                   user_id=user_id,
                   status=status,
                   label_id=service_id)
        db.session.commit()
        flask.current_app.logger.info("Service status {} changed".format(service_id))
    return jsonify(service.as_dict()), 200


@api.route('/service/<string:service_id>/enable', methods=['POST'])
def service_enable(service_id):
    return _set_service_status(service_id, ServiceStatus.OK)


@api.route('/service/<string:service_id>/disable', methods=['POST'])
def service_disable(service_id):
    return _set_service_status(service_id, ServiceStatus.INACTIVE)


@api.route('/search')
def get_search():
    user_id = CONTEXT.auth.get_user_id()
    service_ids = get_user_permissions(user_id).get_permitted_services()

    args = check_args({Required("pattern"): All(basestring,
                                                basestring_strip,
                                                Length(min=2, msg=u"Шаблон должен содержать как минимум 2 непробельных символа")),
                       Optional("offset", default=0): Coerce(int),
                       Optional("limit", default=20): Coerce(int),
                       Optional("active", default=True): Boolean()})

    query = db.session.query(Config).filter(Config.service_id.in_(service_ids)).filter_by(archived=False)

    if args.get("active"):
        statuses = [ConfigStatus.ACTIVE, ConfigStatus.TEST]
        config_ids = db.session.query(Config.id).join(ConfigMark).filter(ConfigMark.status.in_(statuses))
        query = query.filter(Config.id.in_(config_ids))

    pattern = args["pattern"].encode("utf-8")

    def escape_symbols(line):
        line = encode_basestring(line)[1:-1]
        result = r''
        for ch in line:
            if ch == '\\':
                result += r'\\'
            elif ch in '%_':
                result += r'\{}'.format(ch)
            else:
                result += ch
        return result

    if pattern[0] in "'\"" and pattern[-1] in "'\"":
        val = '%{}%'.format(escape_symbols(pattern[1:-1]))
        fulltext = True
    else:
        key_val = pattern.split(': ')
        if len(key_val) != 2:
            val = '%{}%'.format(escape_symbols(pattern))
            fulltext = True
        else:
            key = key_val[0].strip()
            val = '%{}%'.format(escape_symbols(key_val[1].strip()))
            fulltext = False

    val = val.decode("utf-8")
    if fulltext:
        query = query.filter(Config.data.cast(TEXT).ilike(val))
    else:
        query = query.filter(Config.data[key].cast(TEXT).ilike(val))

    overall = query.count()
    items = query.order_by(Config.id.desc()).limit(args["limit"]).offset(args["offset"])

    return jsonify(dict(items=map(_filter_hidden_and_set_defaults_fields, [item.as_dict(with_statuses=True) for item in items]),
                        total=overall)), 200


def _filter_hidden_and_set_defaults_fields(config_dict):
    config_data = TEMPLATE.fill_defaults(config_dict["data"])
    config_dict["data"] = TEMPLATE.filter_hidden_fields(config_dict['service_id'], config_data)
    return config_dict


def _filter_hidden_fields(config_dict):
    config_data = TEMPLATE.filter_hidden_fields(service_id=config_dict.get("service_id"), config_data=config_dict["data"])
    config_dict["data"] = config_data
    return config_dict


@api.route('/service/<string:service_id>/gen_token')
def generate_token(service_id):
    ensure_permissions_on_service(service_id, PermissionKind.TOKEN_UPDATE)
    return jsonify(token=make_token(service_id)), 200


@api.route('/label/<string:label_id>/config/<int:config_id>/moderate', methods=['PATCH'])
def moderate_config(config_id, service_id=None, label_id=None):
    if label_id:
        service_id = get_service_id_for_label(label_id)
    user_id = ensure_permissions_on_service(service_id, PermissionKind.CONFIG_MODERATE)
    args = Schema({Required("approved"): bool,
                   Optional("comment"):
                       All(basestring,
                           Length(min=1, msg=u"Комментарий должен содержать как минимум 1 символ"),
                           Length(max=2048, msg=u"Длина комментария не должна превышать 2048 символов")),
                   })(request.get_json(force=True))

    new_moderate_status = ConfigStatus.APPROVED if args["approved"] else ConfigStatus.DECLINED

    with logging_context(service_id=service_id, config_id=config_id, user_id=user_id):
        if new_moderate_status == ConfigStatus.DECLINED and args.get("comment", "") == "":
            msg = "Couldn't change config moderation status. To decline config needed a comment"
            flask.current_app.logger.info(msg)
            return jsonify(dict(message=u"Нельзя модерировать конфиг. Для отклонения конфига необходим комментарий")), 400

        query = Config.query.outerjoin(ConfigMark).options(contains_eager(Config.statuses)).filter(Config.id == config_id)
        if service_id:
            query = query.filter(Config.service_id == service_id)
        query = query.with_for_update(of=Config).all()
        configs = [config for config in query]
        if len(configs) == 0:
            msg = "Config moderation failed: config not found by id"
            flask.current_app.logger.info(msg)
            return "{}", 404

        config = configs[0]

        if new_moderate_status in config.get_statuses() and new_moderate_status != ConfigStatus.DECLINED:
            msg = "Couldn't change config moderation status. Status already set"
            flask.current_app.logger.info(msg)
            return jsonify(dict(message=u"Нельзя модерировать конфиг. Конфиг уже одобрен")), 400

        if ConfigStatus.ACTIVE in config.get_statuses():
            msg = "Couldn't change moderate status. Config is active"
            flask.current_app.logger.info(msg)
            return jsonify(dict(message=u"Нельзя модерировать конфиг. Конфиг активный.")), 400

        if service_id and not is_service_active(service_id):
            abort(make_response(jsonify(dict(message=SERVICE_IS_INACTIVE_MSG)), 400))
        status_to_remove = filter(lambda s: s.status in {ConfigStatus.APPROVED, ConfigStatus.DECLINED}, config.statuses)
        if len(status_to_remove) > 0:
            db.session.delete(status_to_remove[0])

        config.statuses.append(ConfigMark(status=new_moderate_status, comment=args.get("comment", "")))

        log_action(action=AuditAction.CONFIG_MODERATE, service_id=config.service_id, user_id=user_id,
                   config_id=config_id, moderate=new_moderate_status, comment=args.get("comment", ""),
                   label_id=config.label_id)
        db.session.commit()
        status = "approved" if new_moderate_status == ConfigStatus.APPROVED else "declined"
        flask.current_app.logger.info("Config {} was moderated: {}".format(config_id, status))

    return jsonify(config.as_dict(with_statuses=True)), 200


@api.route('/service/<string:service_id>/comment', methods=['POST'])
def add_comment(service_id):
    user_id = ensure_permissions_on_service(service_id, PermissionKind.SERVICE_COMMENT)
    args = Schema({Optional("comment", default=""):
                  All(basestring,
                      Length(max=1048576, msg=u"Длина комментария не должна превышать 1048576 символов")),
                   })(request.get_json(force=True))
    comment = args.get("comment", "").strip()
    with logging_context(service_id=service_id, user_id=user_id):
        service_comment = ServiceComments.query.filter(ServiceComments.service_id == service_id).with_for_update(of=ServiceComments).first()
        if not service_comment:
            new_service_comment = ServiceComments(service_id=service_id, comment=comment)
            db.session.add(new_service_comment)
        else:
            if comment == service_comment.comment:
                db.session.rollback()
                return "{}", 200
            service_comment.comment = comment
        db.session.commit()
        flask.current_app.logger.info(u"Service {} comment updated".format(service_id), extra=dict(comment=comment))
    return "{}", 200


@api.route('/service/<string:service_id>/set', methods=['POST'])
def set_service_property(service_id):
    user_id = ensure_permissions_on_service(service_id, PermissionKind.SERVICE_MONITORINGS_SWITCH_STATUS)
    args = Schema({
        Optional("monitorings_enabled"): Boolean(),
        Optional("mobile_monitorings_enabled"): Boolean(),
    })(request.get_json(force=True))
    if not args.keys():
        return jsonify(dict(message="No properties were passed")), 400
    monitorings_enabled = args.get("monitorings_enabled")
    mobile_monitorings_enabled = args.get("mobile_monitorings_enabled")

    with logging_context(service_id=service_id, user_id=user_id):
        service = Service.query.filter(Service.id == service_id).with_for_update(of=Service).first_or_404()
        if monitorings_enabled is not None:
            # при изменении monitorings_enabled, меняем на это же значение и mobile_monitorings_enabled
            service.monitorings_enabled = monitorings_enabled
            service.mobile_monitorings_enabled = monitorings_enabled
        elif mobile_monitorings_enabled is not None:
            service.mobile_monitorings_enabled = mobile_monitorings_enabled
        log_action(action=AuditAction.SERVICE_MONITORINGS_SWITCH_STATUS,
                   service_id=service_id, user_id=user_id, label_id=service_id,
                   monitorings_enabled=monitorings_enabled, mobile_monitorings_enabled=mobile_monitorings_enabled)
        db.session.commit()
        flask.current_app.logger.info(u"Service {} monitorings_enabled updated.".format(service_id),
                                      extra=dict(monitorings_enabled=monitorings_enabled, mobile_monitorings_enabled=mobile_monitorings_enabled))
    return "{}", 200


@api.route('/service/<string:service_id>/comment')
def get_comment(service_id):
    ensure_permissions_on_service(service_id, PermissionKind.SERVICE_COMMENT)
    service_comment = ServiceComments.query.filter_by(service_id=service_id).first()
    if not service_comment:
        return jsonify(dict(service_id=service_id, comment='')), 200

    return jsonify(service_comment.as_dict()), 200


@api.route('/get_all_checks')
def get_all_checks():
    user_id = CONTEXT.auth.get_user_id()
    service_ids = get_user_permissions(user_id).get_permitted_services()
    services = Service.query.filter(and_(Service.id.in_(service_ids), Service.monitorings_enabled.is_(True))).order_by(Service.name).all()
    checks_in_progress = {(item.service_id, item.check_id): item
                          for item in ChecksInProgress.query.filter(ChecksInProgress.service_id.in_(service_ids)).filter(ChecksInProgress.time_to >= datetime.utcnow()).all()}
    result = {}
    actual_checks_id = get_actual_dashboard_checks_id()
    for service in services:
        service_result = []
        service_checks = ServiceChecks.query.filter(ServiceChecks.service_id == service.id).\
            order_by(ServiceChecks.group_id).order_by(ServiceChecks.check_id).all()
        for check in service_checks:
            if check.check_id not in actual_checks_id:
                continue
            outdated = 1 if (check.valid_till - datetime.utcnow()).total_seconds() < 0 else 0
            check_result = dict(name=check.check_id, state=check.state, in_progress=bool(checks_in_progress.get((service.id, check.check_id), None)),
                                outdated=outdated, transition_time=int(check.transition_time.strftime("%s")))
            service_result.append(check_result)
        result[service.id] = service_result

    return jsonify(dict(items=result, total=len(result)))


@api.route('/service/<string:service_id>/sonar/rules')
def get_sonar_rules(service_id):
    ensure_permissions_on_service(service_id, PermissionKind.SERVICE_SEE)
    try:
        tags = request.args.get('tags', '')
        tags = tags.split(',') if tags != '' else []
        results = CONTEXT.sonar_client.get_rules_by_service_id(service_id, tags=tags)
    except Exception as e:
        return jsonify({'msg': e.message}), 500
    return jsonify(results), 200


def __get_st_issues(service_id):
    st_client = CONTEXT.startrek_client
    component_id = next((component['id'] for component in st_client.queues[ANTIADB_SUPPORT_QUEUE].components if
                        component['name'] == service_id), None)

    # если у нас нет такого компонента, то и тикетов быть не может -> []
    if component_id is None:
        return [], None

    # Ищем тикеты
    issues = st_client.issues.find(
        filter={
            "queue": ANTIADB_SUPPORT_QUEUE,
            "resolution": "empty()",
            "tags": [TrendType.NEGATIVE_TREND, TrendType.MONEY_DROP],
            "components": [component_id]
        }
    )

    return issues, component_id


@api.route('/service/<string:service_id>/trend_ticket')
def get_service_issue(service_id):
    ensure_permissions_on_service(service_id, PermissionKind.TICKET_SEE)

    # Ищем тикеты
    issues, component_id = __get_st_issues(service_id)

    # у нас нет такого компонента, то и тикетов быть не может -> 400
    if component_id is None:
        return jsonify(dict(message="Wrong component")), 400

    # тикетов нет -> 404
    if len(issues) == 0:
        return jsonify(dict(message="Ticket not found")), 404

    # предполагаем что тикет может быть только 1
    return jsonify({
        'ticket_id': issues[0]['key']
    }), 200


@api.route('/service/<string:service_id>/create_ticket', methods=['POST'])
def create_service_issue(service_id):
    # Создавать тикеты может и робот и человек
    auto_description = ''
    user_id = check_permissions_user_or_service(service_id, PermissionKind.TICKET_CREATE)
    if user_id == 0:
        auto_description = '**Тикет создан мониторингом**\n\n'

    # datetime в формате utc timestamp
    args = Schema({
        Required("type"): Any(TrendType.NEGATIVE_TREND, TrendType.MONEY_DROP),
        Optional("device", default=TrendDeviceType.DESKTOP): Coerce(str),
        Optional("datetime", default=int(time.time())): Coerce(int)
    })(request.get_json(force=True))

    ticket_type = args.get('type')
    device = args.get('device')
    device_tag = 'device:{}'.format(device)

    # берём блокировку на сервис в базе данных, чтобы не создать дубли если мониторинг стригерится дважды
    try:
        create_lock('{}_{}'.format(service_id, device), timeout_seconds=30)
    except Exception as e:
        flask.current_app.logger.info("acquiring lock timed out for service_id={}, device={}".format(service_id, device))
        return jsonify(dict(message=e.message)), 500

    # Ищем тикеты
    issues, component_id = __get_st_issues(service_id)

    if component_id is None:
        return jsonify(dict(message="Wrong component")), 400

    # Получаем приоритет для тикета
    service = Service.query.get_or_404(service_id)
    priority = SUPPORT_PRIORITY_MAP[ticket_type][service.support_priority]

    # Информация принята
    if issues:
        return_code = 208
        existing_issue = issues[0]

        existing_tags = list(existing_issue.tags)
        ticket_type_changed = ticket_type not in existing_tags
        ticket_device_changed = device_tag not in existing_tags
        if ticket_type_changed or ticket_device_changed:
            new_summary = TICKET_TITLES_MAP[ticket_type].format(service_id)
            new_tags = list(set(existing_tags + [ticket_type, device_tag]))
            for tag in new_tags:
                if 'device' in tag:
                    # Добавляем device в описание тикета
                    new_summary = '[{}] '.format(tag.split(':')[1]) + new_summary
            update_args = dict(summary=new_summary, tags=new_tags, ignore_version_change=True)
            if ticket_type_changed and ticket_type == TrendType.MONEY_DROP:
                # При переходе negative_trend -> money_drop меняем приоритет
                update_args.update(priority=priority)
            existing_issue.update(**update_args)
            return_code = 205
        return jsonify({'ticket_id': issues[0]['key']}), return_code

    date = datetime.utcfromtimestamp(args.get('datetime'))

    main_duty, reserved_duty = CONTEXT.get_abc_duty(current_app.config["TOOLS_TOKEN"])

    # Создаем новый тикет
    ticket = CONTEXT.startrek_client.issues.create(
        queue=ANTIADB_SUPPORT_QUEUE,
        summary='[{}] '.format(device) + TICKET_TITLES_MAP[ticket_type].format(service_id),
        tags=[ticket_type, READY_FOR_FILL_TAG, device_tag],
        components=[component_id],
        incidentStart=date.strftime('%Y-%m-%dT%H:%M:%SZ'),
        description=auto_description,
        assignee=main_duty,
        followers=[reserved_duty],
        priority=priority
    )

    # логируем событие
    log_action(action=AuditAction.TICKET_CREATE,
               service_id=service_id,
               label_id=service_id,
               user_id=user_id)

    db.session.commit()

    return jsonify({'ticket_id': ticket['key']}), 201


@api.route('/service/<string:service_id>/support_priority', methods=['PATCH'])
def change_service_priority(service_id):
    user_id = ensure_permissions_on_service(service_id, PermissionKind.SUPPORT_PRIORITY_SWITCH)

    args = Schema({
        Required("support_priority"): Any(*ServiceSupportPriority.all())
    })(request.get_json(force=True))
    support_priority = args.get("support_priority")

    with logging_context(service_id=service_id, user_id=user_id):
        service = Service.query.filter(Service.id == service_id).with_for_update(of=Service).first_or_404()
        old_support_priority = service.support_priority
        service.support_priority = support_priority
        log_action(action=AuditAction.SUPPORT_PRIORITY_SWITCH,
                   service_id=service_id, user_id=user_id, label_id=service_id,
                   support_priority=support_priority, old_support_priority=old_support_priority)
        db.session.commit()
        flask.current_app.logger.info(u"Service {} support_priority updated.".format(service_id),
                                      extra=dict(support_priority=support_priority,
                                                 old_support_priority=old_support_priority))
    return "{}", 200
