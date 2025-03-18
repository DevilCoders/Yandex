# coding=utf-8

"""
Internal API for proxy interaction
"""
import os
import json
from copy import deepcopy
from datetime import datetime
from collections import defaultdict

import re2
from flask import Blueprint, jsonify, request, current_app, g
from sqlalchemy import text, and_
from sqlalchemy.orm import aliased, contains_eager, joinedload
from sqlalchemy.exc import IntegrityError
from voluptuous import Any, Optional, Required, In, Boolean, Schema
from cachetools.func import TTLCache

from antiadblock.configs_api.lib.audit.audit import log_action
from antiadblock.configs_api.lib.auth.auth import ensure_tvm_ticket_is_ok
from antiadblock.configs_api.lib.db import Config, db, Service, ConfigMark, AutoredirectService
from antiadblock.configs_api.lib.db_utils import is_service_active, merge_data, get_label_config_by_id
from antiadblock.configs_api.lib.models import ConfigStatus, ServiceStatus, AuditAction
from antiadblock.configs_api.lib.utils import check_args, cryprox_decrypt_request
from antiadblock.configs_api.lib.jsonlogging import logging_context
from antiadblock.configs_api.lib.context import CONTEXT
from antiadblock.configs_api.lib.const import MAX_URLS_TO_DECRYPT, TOO_MANY_URL_TO_DECRYPT_MSG, DECRYPT_RESPONSE_HEADER_NAME, SERVICE_IS_INACTIVE_MSG, \
    GET_COOKIES_RE, ROOT_LABEL_ID, DEFAULT_COOKIE
from antiadblock.configs_api.lib.validation.template import TEMPLATE, UserDevice
from antiadblock.configs_api.lib.db_utils import get_config_by_status

# TODO: remove when multistatuses is in production
internal_api = Blueprint('internal_api_v2', __name__)

TURBO_REDIRECT_SERVICE_ID = 'autoredirect.turbo'
# no cache for local dev
CACHE_ENABLED = False if os.environ.get("ENVIRONMENT_TYPE", "DEVELOPMENT").upper() == "DEVELOPMENT" else True
CONFIGS_CACHE = TTLCache(maxsize=32, ttl=15)


class InternalApiException(Exception):
    pass


def drop_fields(a_dict):
    return {k: v for (k, v) in a_dict.items() if k not in ['TEST_DATA', 'PARTNER_BACKEND_URL_RE', 'BALANCERS_PROD', 'BALANCERS_TEST']}  # we dont want these passed to proxy


def load_services_configs(statuses, monitorings_enabled=None):
    service_aliased = aliased(Service)
    query = db.session.query(Config, service_aliased.id).join(service_aliased).join(ConfigMark). \
        options(joinedload(Config.statuses)).filter(ConfigMark.status.in_(statuses)). \
        filter(service_aliased.status == ServiceStatus.OK)

    if monitorings_enabled is not None:
        query = query.filter(service_aliased.monitorings_enabled.is_(monitorings_enabled))

    return query.all()


def load_hierarchical_configs(monitorings_enabled=None, replace_configs=None):

    def get_device_type(config_data):
        device = config_data.get('DEVICE_TYPE')
        if device == UserDevice.MOBILE:
            device_type = 'mobile'
        elif device == UserDevice.DESKTOP:
            device_type = 'desktop'
        else:
            device_type = None
        return device_type

    # filter monitoring enabled service
    if monitorings_enabled is None:
        service_filter = "configs.service_id is NULL OR services.status='ok'"
    else:
        value = 't' if monitorings_enabled else 'f'
        service_filter = "configs.service_id is NULL OR (services.status='ok' AND services.monitorings_enabled='{}')".format(value)

    # делаем рекурсивный запрос в БД, чтобы получить все дерево иерархии
    raw_query = text("""
        WITH RECURSIVE r AS (
            SELECT 1 AS level, c.parent_label_id, c.label_id, c.service_id, c.id, c.exp_id, st.status, c.data, c.data_settings
            FROM configs c
                LEFT OUTER JOIN services s ON c.service_id = s.id
                LEFT OUTER JOIN config_statuses st ON c.id = st.config_id
            WHERE (st.status in ('active', 'test') OR c.exp_id is not NULL) AND c.label_id='ROOT'
            UNION ALL
            SELECT r.level+1 as level, configs.parent_label_id, configs.label_id, configs.service_id, configs.id, configs.exp_id, config_statuses.status,
                            configs.data, configs.data_settings
            FROM configs
                LEFT OUTER JOIN services ON configs.service_id = services.id
                LEFT OUTER JOIN config_statuses ON configs.id = config_statuses.config_id
                JOIN r ON configs.parent_label_id = r.label_id
            WHERE (config_statuses.status in ('active', 'test') OR configs.exp_id is not NULL) AND ({service_filter})
        )
        SELECT * FROM r;
        """.format(service_filter=service_filter))
    results = db.engine.execute(raw_query)
    configs = {ConfigStatus.ACTIVE: defaultdict(dict), ConfigStatus.TEST: defaultdict(dict)}
    for row in results:
        if row.status in (ConfigStatus.ACTIVE, ConfigStatus.TEST):
            key = row.status
        elif row.exp_id is not None:
            key = row.exp_id
            if key not in configs:
                configs[key] = defaultdict(dict)
        else:
            msg = "Config without status and exp_id"
            current_app.logger.exception(msg)
            raise InternalApiException(msg)

        configs[key][int(row.level)][row.label_id] = dict(row)
        # replace configs https://st.yandex-team.ru/ANTIADB-2743
        if replace_configs is not None:
            if row.label_id in replace_configs:
                replaced = get_label_config_by_id(replace_configs[row.label_id], row.label_id)
                if replaced:
                    current_app.logger.info("Replace configs for label '{}', using #{}".format(row.label_id, replace_configs[row.label_id]))
                    for _key in ("id", "data", "data_settings"):
                        configs[key][int(row.level)][row.label_id][_key] = replaced.as_dict()[_key]

    result_configs = {}

    for key in set(configs.keys()):
        for level in sorted(configs[key].keys()):
            if level == 1:
                continue
            status, exp_id = None, None
            if key in (ConfigStatus.ACTIVE, ConfigStatus.TEST):
                status = key
            else:
                exp_id = key
            for label in configs[key][level].keys():
                config = configs[key][level][label]
                parent = configs[key][level-1][config['parent_label_id']] if level-1 in configs[key] else configs[ConfigStatus.ACTIVE][level-1][config['parent_label_id']]
                config['data'] = merge_data(parent['data'], config['data'], config['data_settings'])
                if config.get('service_id') is not None:
                    result_configs[(config['service_id'], status, get_device_type(config['data']), exp_id)] = {'config': config['data'], 'version': config['id']}

    return result_configs


def add_autoredirect_data(a_dict, service_id=TURBO_REDIRECT_SERVICE_ID):
    if service_id not in a_dict:
        return
    a_dict[service_id]['config']['WEBMASTER_DATA'] = {
        service.webmaster_service_id: {
            "domain": service.domain,
            "urls": service.urls,
        }
        for service in AutoredirectService.query.all()
    }


# deprecate handler
@internal_api.route('/configs')
def all_configs_v2():
    ensure_tvm_ticket_is_ok(CONTEXT,
                            allowed_apps=["AAB_CRYPROX_TVM_CLIENT_ID", "AAB_SANDBOX_MONITORING_TVM_ID",
                                          "AAB_CHECKLIST_JOB_TVM_CLIENT_ID", "MONRELAY_TVM_CLIENT_ID"])

    args = check_args({
        Optional("status"): Any(*ConfigStatus.all()),
        Optional("monitorings_enabled"): Boolean(),
    })
    if not args.get("status"):
        statuses = [ConfigStatus.ACTIVE, ConfigStatus.TEST]
    else:
        statuses = [args["status"]]

    query_results = load_services_configs(statuses, args.get("monitorings_enabled"))

    result = {}
    for config, service_id in query_results:
        with logging_context(config_id=config.id, service_id=service_id):
            config_model = dict(version=config.id,
                                statuses=map(lambda cm: cm.status, config.statuses),
                                config=drop_fields(TEMPLATE.fill_defaults(config.data)))
            if service_id not in result:
                result[service_id] = [config_model]
            else:
                result[service_id].append(config_model)
    return jsonify(result)


# new handler
@internal_api.route('/configs_handler')
def all_configs_v2_new_handler():
    ensure_tvm_ticket_is_ok(CONTEXT,
                            allowed_apps=["AAB_CRYPROX_TVM_CLIENT_ID", "AAB_SANDBOX_MONITORING_TVM_ID", "AAB_CHECKLIST_JOB_TVM_CLIENT_ID", "MONRELAY_TVM_CLIENT_ID"])

    args = check_args({
        Required("status"): In((ConfigStatus.ACTIVE, ConfigStatus.TEST)),
        Optional("monitorings_enabled"): Boolean(),
    })

    key = "configs_handler_{}_{}".format(args["status"], args.get("monitorings_enabled"))
    cached_value = CONFIGS_CACHE.get(key)
    if cached_value:
        return cached_value

    configs = load_hierarchical_configs(monitorings_enabled=args.get("monitorings_enabled"))

    result = {}
    for (service_id, status, device_type, exp_id), v in configs.iteritems():
        if status != args["status"] or device_type is not None:
            continue
        with logging_context(config_id=v["version"], service_id=service_id):
            config_model = dict(version=v["version"],
                                statuses=[args["status"]],
                                config=drop_fields(TEMPLATE.fill_defaults(v["config"])))
            result[service_id] = config_model

    add_autoredirect_data(result)
    result = jsonify(result)
    if CACHE_ENABLED:
        CONFIGS_CACHE[key] = result
    return result


# new handler for Sandbox task monitorings_update
@internal_api.route('/monitoring_settings')
def monitoring_settings():
    """
    :return: dict(service_1 = <list of devices>, ...)
    """
    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=["AAB_CRYPROX_TVM_CLIENT_ID", "AAB_SANDBOX_MONITORING_TVM_ID"])

    result = {}
    services = Service.query.filter(and_(Service.monitorings_enabled.is_(True), Service.status == 'ok')).order_by(Service.name).all()
    for service in services:
        result[service.id] = ['desktop']
        if service.mobile_monitorings_enabled:
            result[service.id].append('mobile')

    return jsonify(result)


# new hierarchical handler
@internal_api.route('/configs_hierarchical_handler')
def hierarchical_configs_handler():
    ensure_tvm_ticket_is_ok(CONTEXT,
                            allowed_apps=["AAB_CRYPROX_TVM_CLIENT_ID", "AAB_SANDBOX_MONITORING_TVM_ID"])
    args = check_args({
        Optional("monitorings_enabled"): Boolean(),
        Optional("replace_configs"): basestring,
    })

    # TODO: write tests https://st.yandex-team.ru/ANTIADB-2746
    replace_configs = None
    if "replace_configs" in args:
        try:
            current_app.logger.info("replace_configs: {}".format(args["replace_configs"]))
            replace_configs = json.loads(args["replace_configs"])
        except Exception as e:
            current_app.logger.warning("Can't loads replace_configs, error: {}".format(str(e)))

    key = "configs_handler_{}".format(args.get("monitorings_enabled"))
    cached_value = CONFIGS_CACHE.get(key)
    if cached_value and replace_configs is None:
        return cached_value

    configs = load_hierarchical_configs(monitorings_enabled=args.get("monitorings_enabled"), replace_configs=replace_configs)
    result = {"::".join([str(i) for i in k]): {"config": drop_fields(TEMPLATE.fill_defaults(v["config"])), "version": v["version"]} for k, v in configs.iteritems()}
    add_autoredirect_data(result, TURBO_REDIRECT_SERVICE_ID + "::active::None::None")
    result = jsonify(result)
    if CACHE_ENABLED:
        CONFIGS_CACHE[key] = result
    return result


@internal_api.route('/decrypt_urls/<string:service_id>', methods=['POST'])
def decrypt_urls_handler(service_id):
    """
        Handler for external clients. E.g. ErrorBooster
    """
    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=("AAB_CRYPROX_TVM_CLIENT_ID", "AAB_SANDBOX_MONITORING_TVM_ID", "ERROR_BUSTER_TVM_CLIENT_ID", "ANTIADB_SUPPORT_BOT_TVM_CLIENT_ID"))
    if not is_service_active(service_id):
        return jsonify({'message': SERVICE_IS_INACTIVE_MSG, 'request_id': g.request_id}), 400
    urls = request.get_json(force=True).get('urls', list())
    if len(urls) > MAX_URLS_TO_DECRYPT:
        return jsonify({'message': TOO_MANY_URL_TO_DECRYPT_MSG, 'request_id': g.request_id}), 400
    decrypted_urls = list()
    for crypted_url in urls:
        decrypted_url = cryprox_decrypt_request(service_id, crypted_url, CONTEXT.cryprox, current_app).headers.get(DECRYPT_RESPONSE_HEADER_NAME)
        decrypted_urls.append(decrypted_url)
    return jsonify({'urls': decrypted_urls})


@internal_api.route('/redirect_data', methods=['POST'])
def redirect_data_handler():
    """
        Handler for SB-task, add WEBMASTER_DATA to BD
        {
        webmaster_service_id_1: {"domian": domain_1, "urls": [<list_urls>]},
        webmaster_service_id_2: {"domian": domain_2, "urls": [<list_urls>]},
        ...
        }
    """
    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=["AAB_SANDBOX_MONITORING_TVM_ID", "AAB_CRYPROX_TVM_CLIENT_ID"])
    webmaster_data = request.get_json(force=True)
    if not isinstance(webmaster_data, dict):
        return jsonify({'message': 'Invalid data format'}), 400

    known_services = {service.webmaster_service_id: service for service in AutoredirectService.query.all()}

    try:
        # delete old services, update existing, append new
        for webmaster_service_id, service in known_services.iteritems():
            if webmaster_service_id not in webmaster_data:
                db.session.delete(service)

        for webmaster_service_id, values in webmaster_data.iteritems():
            old_service = known_services.get(webmaster_service_id)
            if old_service is not None:
                old_service.webmaster_service_id = webmaster_service_id
                old_service.domain = values["domain"]
                old_service.urls = values["urls"]
            else:
                db.session.add(AutoredirectService(webmaster_service_id=webmaster_service_id, domain=values["domain"], urls=values["urls"]))

        db.session.commit()
        current_app.logger.debug("Webmaster services updated")
        return '{}', 201
    except Exception as e:
        db.session.rollback()
        current_app.logger.error("Services checks failed", extra=dict(error=str(e)))
        return jsonify({'message': str(e)}), 500


@internal_api.route('/change_current_cookie', methods=['POST'])
def change_current_cookie_handler():
    """
    Handler for SB-task, change CURRENT_COOKIE for service_id
    Request json
    {
        service_id: <service_id>,
        test: true|false,
    }
    """
    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=['AAB_SANDBOX_MONITORING_TVM_ID', 'AAB_CRYPROX_TVM_CLIENT_ID'])
    args = Schema({Required('service_id'): basestring,
                   Optional('test', default=True): Boolean()})(request.get_json(force=True))

    if not is_service_active(args['service_id']):
        return jsonify({'message': SERVICE_IS_INACTIVE_MSG, 'request_id': g.request_id}), 400

    status = ConfigStatus.TEST if args['test'] else ConfigStatus.ACTIVE

    try:
        # найдем уровень, на котором определена кука дня
        label_id = args['service_id']
        while label_id != ROOT_LABEL_ID:
            old_config = get_config_by_status(service_id=None, status=status, label_id=label_id)
            old_config_data = old_config.data
            current_cookie = old_config_data.get('CURRENT_COOKIE')
            if current_cookie is not None and current_cookie != DEFAULT_COOKIE:
                break
            label_id = old_config.parent_label_id
        else:
            msg = 'No cookie of the day on service'
            current_app.logger.info(msg, extra=dict(service_id=args['service_id']))
            return jsonify({'message': msg}), 200

        # вытащим все текущие партнерские правила на удаление куки
        rules = [rule['raw_rule'] for rule in CONTEXT.sonar_client.get_partner_rules(args['service_id'])
                 if any([el in rule['raw_rule'] for el in ('$cookie=', 'AG_removeCookie', 'cookie-remover')])]
        if not rules:
            msg = 'No cookie remover rules'
            current_app.logger.warning(msg, extra=dict(service_id=args['service_id']))
            return jsonify({'message': msg, 'request_id': g.request_id}), 400

        cookie_rules_re = re2.compile('|'.join([GET_COOKIES_RE.match(rule).group('cookies').replace('\$', '$') for rule in rules]))
        cookie_matcher = cookie_rules_re.match(current_cookie)
        if not cookie_matcher:
            msg = 'Current cookie not matched for actual rules'
            current_app.logger.info(msg, extra=dict(service_id=args['service_id']))
            return jsonify({'message': msg, 'current_cookie': old_config_data['CURRENT_COOKIE']}), 200
        # определяем следующую куку
        curr_index = old_config_data['WHITELIST_COOKIES'].index(current_cookie)
        whitelist = old_config_data['WHITELIST_COOKIES'][curr_index+1:] + old_config_data['WHITELIST_COOKIES'][:curr_index]

        available_cookies_count = 0
        new_cookie = ""
        for cookie in whitelist:
            if not cookie_rules_re.match(cookie):
                available_cookies_count += 1
                new_cookie = new_cookie or cookie
        if not new_cookie:
            msg = 'All cookies matched for actual rules'
            current_app.logger.warning(msg, extra=dict(service_id=args['service_id']))
            return jsonify({'message': msg,  'request_id': g.request_id}), 400

        new_config_data = deepcopy(old_config_data)
        if 'DEPRECATED_COOKIES' in new_config_data:
            new_config_data['DEPRECATED_COOKIES'].append(current_cookie)
        else:
            new_config_data['DEPRECATED_COOKIES'] = [current_cookie]

        if new_cookie in new_config_data['DEPRECATED_COOKIES']:
            new_config_data['DEPRECATED_COOKIES'].remove(new_cookie)
        new_config_data['CURRENT_COOKIE'] = new_cookie

        new_config = Config(
            comment='New cookie {}'.format(new_cookie),
            data=new_config_data,
            data_settings=old_config.data_settings,
            created=datetime.utcnow(),
            service_id=old_config.service_id,
            creator_id=None,
            parent_id=old_config.id,
            label_id=label_id,
            parent_label_id=old_config.parent_label_id,
        )

        status_to_remove = filter(lambda s: s.status == status, old_config.statuses)[0]
        db.session.delete(status_to_remove)
        new_config.statuses.append(ConfigMark(status=status))
        new_config.statuses.append(ConfigMark(status=ConfigStatus.APPROVED))

        db.session.add(new_config)
        db.session.flush()

        log_action(action=AuditAction.CONFIG_TEST if args['test'] else AuditAction.CONFIG_ACTIVE,
                   service_id=args['service_id'],
                   user_id=None,
                   config_id=new_config.id,
                   old_config_id=old_config.id,
                   config_comment=new_config.comment,
                   label_id=label_id,
                   new_cookie=new_cookie)
        db.session.commit()
        current_app.logger.info("Cookie changed", extra=dict(service_id=args['service_id'], status=status))
        return jsonify({"new_cookie": new_cookie, "available_cookies_count": available_cookies_count - 1, "label_id": label_id}), 201
    except IntegrityError as ex:
        db.session.rollback()
        msg = u"Обнаружены параллельные изменения конфигов"
        current_app.logger.info(msg, extra=dict(service_id=args['service_id'], status=status))
        current_app.logger.error(ex)
        return jsonify(dict(message=msg)), 409
    except Exception as e:
        db.session.rollback()
        current_app.logger.error('Change current cookie failed', extra=dict(error=str(e), service_id=args['service_id'], status=status))
        return jsonify({'message': str(e), 'request_id': g.request_id}), 500


@internal_api.route('/services/balancer_settings')
def get_services_balancers_settings():
    PROD, TEST = 'prod', 'test'

    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=["AAB_SANDBOX_MONITORING_TVM_ID", "AAB_CRYPROX_TVM_CLIENT_ID"])
    configs_by_services = defaultdict(dict)
    hierarchical_configs = load_hierarchical_configs()
    for key, value in hierarchical_configs.items():
        service_id, status, _, _ = key
        if status == ConfigStatus.ACTIVE and 'BALANCERS_PROD' in value['config']:
            configs_by_services[service_id][PROD] = value['config']
        elif status == ConfigStatus.TEST and 'BALANCERS_TEST' in value['config']:
            configs_by_services[service_id][TEST] = value['config']

    result = {}
    for service_id, configs in configs_by_services.items():
        balancer_settings = {}
        for env, config in configs.items():
            balancer_settings[env] = dict(
                crypt_enable_trailing_slash=config.get('CRYPT_ENABLE_TRAILING_SLASH', False),
                crypt_preffixes='|'.join(re2.escape(preffix) for preffix in [config.get('CRYPT_URL_PREFFIX', '/')] + config.get('CRYPT_URL_OLD_PREFFIXES', [])),
                crypt_secret_key=config.get('CRYPT_SECRET_KEY'),
                backend_url_re='|'.join(config.get('PARTNER_BACKEND_URL_RE', ())),
                service_id=service_id,
            )

        result[service_id] = {}
        if PROD in balancer_settings:
            for balancer in configs[PROD]['BALANCERS_PROD']:
                result[service_id][balancer] = balancer_settings[PROD]
        if TEST in balancer_settings:
            for balancer in configs[TEST]['BALANCERS_TEST']:
                result[service_id][balancer] = balancer_settings[TEST]

    return jsonify(result)
