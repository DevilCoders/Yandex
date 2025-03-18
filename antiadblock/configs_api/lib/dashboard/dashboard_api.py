# coding=utf-8
from datetime import datetime, timedelta

from flask import Blueprint, jsonify, current_app, request, g
from voluptuous import Schema, Optional, All, Range


from antiadblock.configs_api.lib.auth.auth import ensure_tvm_ticket_is_ok
from antiadblock.configs_api.lib.auth.permission_models import PermissionKind, ROLES_WEIGHT, DEFAULT_ADMIN_USERNAME
from antiadblock.configs_api.lib.auth.permissions import ensure_permissions_on_service, get_user_role_weight
from antiadblock.configs_api.lib.context import CONTEXT
from antiadblock.configs_api.lib.db import db, ServiceChecks, ChecksInProgress, UserLogins, Service
from antiadblock.configs_api.lib.auth.nodes import Root

from .config import DASHBOARD_API_CONFIG


dashboard_api = Blueprint('dashboard_api', __name__)


def get_actual_dashboard_checks_id():
    checks_id = set()
    for group in DASHBOARD_API_CONFIG["groups"]:
        for check in group["checks"]:
            checks_id.add(check["check_id"])
    return checks_id


@dashboard_api.route('/get_checks_config', methods=['GET'])
def get_checks_config():
    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=['MONRELAY_TVM_CLIENT_ID'])
    return jsonify(current_app.config["DASHBOARD_API_CONFIG"])


@dashboard_api.route('/service/<string:service_id>/get_service_checks', methods=['GET'])
def get_service_checks(service_id):
    user_id = ensure_permissions_on_service(service_id, PermissionKind.SERVICE_SEE)
    self_role_weight = get_user_role_weight([user_id], Root.SERVICES[service_id]).get(user_id, ROLES_WEIGHT["external_user"])
    service_checks = ServiceChecks.query.filter_by(service_id=service_id).join(Service).filter(Service.monitorings_enabled.is_(True)).all()
    checks_in_progress = {item.check_id: item for item in ChecksInProgress.query.filter_by(service_id=service_id).filter(ChecksInProgress.time_to >= datetime.utcnow()).all()}

    service_checks_dict = dict()
    for row in service_checks:
        service_checks_dict[row.check_id] = {
            'state': row.state,
            'value': row.value,
            'external_url': row.external_url,
            'last_update': row.last_update,
            'valid_till': row.valid_till,
            'transition_time': row.transition_time,
            'in_progress': False,
        }
        check = checks_in_progress.get(row.check_id, None)
        if check is not None:
            # для пользователей с правами меньше админских скрываем имя пользователя
            login = check.login if self_role_weight >= ROLES_WEIGHT["admin"] else DEFAULT_ADMIN_USERNAME
            service_checks_dict[row.check_id].update({
                'in_progress': True,
                'login': login,
                'time_from': check.time_from,
                'time_to': check.time_to,
            })

    result = list()
    for group in DASHBOARD_API_CONFIG['groups']:
        result_group = {
            'group_id': group['group_id'],
            'group_title': group['group_title'],
            'checks': list()
        }
        for check in group.get('checks', list()):
            db_fields = service_checks_dict.get(check['check_id'], None)
            if db_fields is None:
                # Check in config but not in db
                continue
            check_result = {
                'check_id': check['check_id'],
                'check_title': check['check_title'],
                'state': db_fields['state'],
                'value': db_fields['value'],
                'external_url': db_fields['external_url'],
                'description': check['description'],
                'last_update': db_fields['last_update'],
                'valid_till': db_fields['valid_till'],
                'transition_time': db_fields['transition_time'],
                'in_progress': db_fields['in_progress'],
            }
            if db_fields['in_progress']:
                check_result['progress_info'] = {
                    'login': db_fields['login'],
                    'time_from': db_fields['time_from'],
                    'time_to': db_fields['time_to'],
                }
            result_group['checks'].append(check_result)

        if result_group['checks']:
            # Only groups which have check results
            result.append(result_group)

    return jsonify({'groups': result})


@dashboard_api.route('/post_service_checks', methods=['POST'])
def post_service_checks():
    """
    Формат принимаемых данных
    [{service_id: str(max 127 symbols), group_id: str(max 127 symbols), check_id: str(max 127 symbols),
      state: str(max 127 symbols), state: str(max 255 symbols), external_url: str(max 4096 symbols),
      last_update: timestamp (YYYY-MM-DD hh:mm:ss), valid_till: timestamp (YYYY-MM-DD hh:mm:ss)},
      {...},
    ]
    """
    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=["MONRELAY_TVM_CLIENT_ID"])

    all_checks = request.get_json(force=True)
    if not isinstance(all_checks, list):
        return jsonify({'message': 'Invalid data format'}), 400

    service_ids = [check['service_id'] for check in all_checks]
    all_service_check = {(item.service_id, item.check_id): item
                         for item in ServiceChecks.query.filter(ServiceChecks.service_id.in_(service_ids)).all()}

    try:
        for check in all_checks:
            service_check = all_service_check.get((check['service_id'], check['check_id']), None)
            if service_check is None:
                service_check = ServiceChecks(service_id=check['service_id'], group_id=check['group_id'], check_id=check['check_id'],
                                              state=check['state'], value=check['value'], external_url=check['external_url'],
                                              last_update=check['last_update'], valid_till=check['valid_till'], transition_time=check['last_update'])
                db.session.add(service_check)
                current_app.logger.debug("Service check appended", extra=dict(service_check=check))
            else:
                if check['state'] != service_check.state:
                    service_check.transition_time = check['last_update']
                    service_check.state = check['state']

                service_check.value = check['value']
                service_check.external_url = check['external_url']
                service_check.last_update = check['last_update']
                service_check.valid_till = check['valid_till']

        db.session.commit()
        current_app.logger.debug("Services check updated")
        return '{}', 201
    except Exception as e:
        db.session.rollback()
        current_app.logger.error("Services checks failed", extra=dict(error=str(e)))
        return jsonify({'message': str(e)}), 500


@dashboard_api.route('/service/<string:service_id>/check/<string:check_id>/in_progress', methods=['PATCH'])
def patch_check_in_progress(service_id, check_id):
    user_id = ensure_permissions_on_service(service_id, PermissionKind.SERVICE_CHECK_UPDATE)
    args = Schema({Optional("hours", default=24): All(int,
                                                      Range(min=1, max=168, msg=u"Аргумент должен быть в диапазоне от 1 до 168")
                                                      )})(request.get_json(force=True))

    service_checks = ServiceChecks.query.filter_by(service_id=service_id).filter_by(check_id=check_id).first()
    if not service_checks:
        msg = "Not found service_check"
        current_app.logger.error(msg, extra=dict(service_id=service_id, check_id=check_id, user_id=user_id))
        return jsonify(dict(message=msg)), 404

    user_login = UserLogins.query.filter_by(uid=user_id).first()
    login = user_login.passportlogin

    check_in_progress = ChecksInProgress.query.filter_by(service_id=service_id).filter_by(check_id=check_id).first()

    time_from = datetime.utcnow()
    time_to = (time_from + timedelta(hours=args['hours'])).strftime("%Y-%m-%dT%TZ")
    time_from = time_from.strftime("%Y-%m-%dT%TZ")

    try:
        if not check_in_progress:
            check_in_progress = ChecksInProgress(service_id=service_id, check_id=check_id, login=login,
                                                 time_from=time_from, time_to=time_to)
            db.session.add(check_in_progress)
        else:
            check_in_progress.login = login
            check_in_progress.time_from = time_from
            check_in_progress.time_to = time_to

        db.session.commit()
        current_app.logger.debug("Set status in progress", extra=dict(service_id=service_id, check_id=check_id, login=login))
    except Exception as e:
        db.session.rollback()
        msg = "Cannot set status in progress due to internal error"
        current_app.logger.error(msg, extra=dict(service_id=service_id, check_id=check_id, login=login, error=str(e)))
        return jsonify(dict(message=msg, request_id=g.request_id)), 500

    return '{}', 201
