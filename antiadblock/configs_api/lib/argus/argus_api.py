# coding=utf-8
from datetime import datetime, timedelta
from collections import defaultdict
from copy import deepcopy

import requests
import flask
from flask import Blueprint, jsonify, request, abort, make_response
from voluptuous import Schema, Optional, Required, Coerce, All, Any, Invalid, Boolean
from sqlalchemy import asc, desc, or_, text
from urllib import quote

from antiadblock.configs_api.lib.auth.auth import ensure_tvm_ticket_is_ok
from antiadblock.configs_api.lib.auth.permissions import ensure_permissions_on_service, check_permissions_user_or_service, \
    ensure_user_admin
from antiadblock.configs_api.lib.auth.permission_models import PermissionKind
from antiadblock.configs_api.lib.context import CONTEXT
from antiadblock.configs_api.lib.validation.template import validate_ts

from antiadblock.configs_api.lib.utils import check_args
from antiadblock.configs_api.lib.db_utils import get_active_config, get_data_from_parent_configs

from antiadblock.configs_api.lib.db import Config, db, SBSProfiles, SBSRuns, SBSResults, UserLogins
from antiadblock.configs_api.lib.models import SbSCheckStatus, ArgusLogStatus

from antiadblock.configs_api.lib.service_api.api import basestring_strip

argus_api = Blueprint('argus_api', __name__)


def basestring_encode(value):
    if isinstance(value, str):
        return value
    return value.encode('utf-8')


def find_user_name(user_id):
    if user_id == 0:
        return 'Mr Robot'
    return UserLogins.query.get_or_404(user_id).internallogin


def _get_last_sbs_profile(service_id, empty_profile=False, tag="default"):
    result = SBSProfiles.query.filter_by(service_id=service_id, tag=tag, is_archived=False).first()
    if result is None:
        if not empty_profile:
            abort(make_response(jsonify(dict(message="Not found sbs profile in service {}".format(service_id)))), 404)
        return dict(
            id=0,
            service_id=service_id,
            date=datetime.utcnow(),
            data=dict(
                general_settings={},
                url_settings=[],
            ),
        )
    else:
        return result.as_dict()


@argus_api.route('/service/<string:service_id>/sbs_check/run', methods=["POST"])
def run_sbs_check(service_id):
    user_id = check_permissions_user_or_service(service_id, PermissionKind.SBS_RUN_CHECK)
    args = Schema({
        Optional('exp', default=""): basestring,
        Optional('testing', default=False): Boolean(),
        Optional('tag', default="default"): All(basestring, basestring_strip),
        Optional('argus_resource_id'): Coerce(int),
    })(request.get_json(force=True))

    flask.current_app.logger.info("exp: {}, testing: {}\n".format(args.get('exp'), args.get('testing')))
    if args.get('exp') and args.get('testing'):
        raise Invalid(u"Невозможен запуск и тестового и экспериментального конфига одновременно")

    config_id = get_active_config(service_id).id
    config = Config.query.filter_by(service_id=service_id, id=config_id).first_or_404()

    profile = _get_last_sbs_profile(service_id, tag=args["tag"])

    argus_resource_id = args.get('argus_resource_id')
    if argus_resource_id is not None:
        flask.current_app.logger.info("test run with resource id {}\n".format(argus_resource_id))

    check = SBSRuns(
        status=SbSCheckStatus.NEW,
        owner=user_id,
        date=datetime.utcnow(),
        config_id=config_id,
        sandbox_id=0,
        profile_id=profile["id"],
        is_test_run=argus_resource_id is not None,
    )
    db.session.add(check)
    db.session.commit()

    def get_proxy_cookies(config_dict, parent_label_id):
        data = config_dict['data']
        parent_data, _ = get_data_from_parent_configs(config_dict["service_id"], parent_label_id, None)

        current_cookie = data.get('CURRENT_COOKIE')
        if parent_data and current_cookie is None:
            current_cookie = parent_data.get('CURRENT_COOKIE')

        exclude_cookies = data.get('EXCLUDE_COOKIE_FORWARD')
        if parent_data and exclude_cookies is None:
            exclude_cookies = parent_data.get('EXCLUDE_COOKIE_FORWARD')

        exclude_cookies = exclude_cookies or []
        exclude_cookies.append(current_cookie)
        cookie_string = ""
        for cookie in exclude_cookies:
            if cookie_string:
                cookie_string += ";"
            cookie_string += "{}=1".format(cookie)
        return cookie_string

    def get_merged_urls_and_general_settings(_profile, config, additional_params):
        new_profile = {
            "url_settings": _profile["data"]["url_settings"],
            "cookies": _profile["data"]["general_settings"].get("cookies", ""),
            "proxy_cookies": get_proxy_cookies(config.as_dict(), config.parent_label_id),
            "headers": _profile["data"]["general_settings"].get("headers", {}),
            "filters_list": _profile["data"]["general_settings"].get("filters_list", [])
        }

        # experiment run
        if additional_params.get("exp"):
            new_profile["headers"]["x-aab-exp-id"] = additional_params["exp"]

        # testing run
        if additional_params.get("testing"):
            new_profile["cookies"] = "aabtesting=1;" + new_profile["cookies"]

        for item in new_profile["url_settings"]:
            item["url"] = quote(item["url"].encode('utf-8'), safe='/:?=%@;&+$,')
            item["selectors"] = item.get("selectors", [])
            item["selectors"].extend(_profile["data"]["general_settings"].get("selectors", []))
            item["wait_sec"] = item.get('wait_sec', _profile["data"]["general_settings"].get("wait_sec", 0))
            item["scroll_count"] = item.get('scroll_count', _profile["data"]["general_settings"].get("scroll_count", 0))
        return new_profile
    try:
        task_id = CONTEXT.argus_client.run_argus_task(
            get_merged_urls_and_general_settings(profile, config, args),
            check.id,
            argus_resource_id=argus_resource_id,
        )
    except Exception as e:
        return jsonify({"msg": e.message}), 500

    check.sandbox_id = task_id
    check.status = SbSCheckStatus.IN_PROGRESS
    db.session.commit()

    return jsonify({"id": task_id, "run_id": check.id}), 201


@argus_api.route('/service/<string:service_id>/sbs_check/results')
def get_sbs_result_list(service_id):
    user_id = ensure_permissions_on_service(service_id, PermissionKind.SBS_RESULTS_SEE)
    args = check_args({Optional("offset", default=0): Coerce(int),
                       Optional("limit", default=20): Coerce(int),
                       Optional("only_my_runs", default=False): Coerce(bool),
                       Optional("status"): Any(*SbSCheckStatus.all()),
                       Optional("from"): validate_ts,  # дата в формате ISO 8601
                       Optional("to"): validate_ts,
                       Optional("sortedby", default='date'): All(basestring, basestring_strip),  # колонка в таблице
                       Optional("order", default='desc'): All(basestring, basestring_strip)})  # порядок сортировки
    datetime_format = '%Y-%m-%dT%H:%M:%S'
    filter_conditions = ["configs.service_id = '{}'".format(service_id), "NOT sbs_runs.is_test_run"]
    if args.get("status", '').strip() not in ('all', ''):
        filter_conditions.append("sbs_runs.status = '{}'".format(args["status"]))
    if args.get("only_my_runs"):
        filter_conditions.append("sbs_runs.owner = {}".format(user_id))
    if args.get("from"):
        filter_conditions.append("sbs_runs.date >= '{}'".format(args["from"]))
    if args.get("to"):
        filter_conditions.append("sbs_runs.date <= '{}'".format(
            (datetime.strptime(args["to"], datetime_format) + timedelta(days=1)).isoformat()))

    def get_filter_condition_query(conditions):
        result = ""
        is_first = True
        for cond in conditions:
            if is_first:
                result = "WHERE " + cond + " "
                is_first = False
            else:
                result += "AND " + cond + " "
        return result

    select_count_sql = "SELECT COUNT(*) FROM sbs_runs "
    select_raw_sql = """
    SELECT
        sbs_runs.id, sbs_runs.status, sbs_runs.owner, sbs_runs.date, configs.id as config_id, sbs_runs.sandbox_id,
        sbs_runs.profile_id, configs.service_id, t2.ok_cases, t2.problem_cases, t2.unknown_cases, t2.obsolete_cases,
        t2.no_reference_cases, t2.new_cases
    FROM sbs_runs
    """

    configs_join_sql = " JOIN configs ON sbs_runs.config_id = configs.id "
    filter_conditions_sql = get_filter_condition_query(filter_conditions)
    outer_join_sql = """
    left join lateral (
         SELECT
            id as join_id,
            SUM(CASE WHEN obj->>'has_problem' = 'new' THEN 1 ELSE 0 END) as new_cases,
            SUM(CASE WHEN obj->>'has_problem' = 'ok' THEN 1 ELSE 0 END) as ok_cases,
            SUM(CASE WHEN obj->>'has_problem' = 'problem' THEN 1 ELSE 0 END) as problem_cases,
            SUM(CASE WHEN obj->>'has_problem' = 'unknown' THEN 1 ELSE 0 END) as unknown_cases,
            SUM(CASE WHEN obj->>'has_problem' = 'obsolete' THEN 1 ELSE 0 END) as obsolete_cases,
            SUM(CASE WHEN obj->>'has_problem' = 'no_reference_case' THEN 1 ELSE 0 END) as no_reference_cases
        FROM sbs_results, jsonb_array_elements(sbs_results.cases) as obj
        WHERE sbs_results.id = sbs_runs.id
        GROUP BY id
    ) as t2
    on true """

    order_limit_part = " ORDER BY {sortedby} {direction} LIMIT {limit} OFFSET {offset} ".format(
        direction="DESC" if args['order'] == "desc" else "ASC",
        limit=args["limit"], offset=args["offset"],
        sortedby="config_id" if args["sortedby"] == "config_id" else "sbs_runs." + args["sortedby"]
    )

    count_query = text(select_count_sql + configs_join_sql + filter_conditions_sql)
    overall = None
    for item in db.engine.execute(count_query):
        overall = item.count

    raw_query = text(select_raw_sql + configs_join_sql + outer_join_sql + filter_conditions_sql + order_limit_part)

    items = db.engine.execute(raw_query)

    items_list = []
    for item in items:
        items_list.append({
            "id": item.id,
            "status": item.status,
            "owner": find_user_name(item.owner),
            "date": item.date,
            "config_id": item.config_id,
            "sandbox_id": item.sandbox_id,
            "profile_id": item.profile_id,
            "new_cases": item.new_cases or 0,
            "ok_cases": item.ok_cases or 0,
            "problem_cases": item.problem_cases or 0,
            "unknown_cases": item.unknown_cases or 0,
            "obsolete_cases": item.obsolete_cases or 0,
            "no_reference_cases": item.no_reference_cases or 0,
        })

    answer = {
        "schema": {
            "id": "run-id",
            "date": "date",
            "config_id": "config-id",
            "sandbox_id": "sandbox-id",
            "owner": "owner",
            "status": "status",
            "profile_id": "profile-id",
            "ok_cases": "ok-cases",
            "new_cases": "new-cases",
            "problem_cases": "problem-cases",
            "unknown_cases": "unknown-cases",
            "obsolete_cases": "obsolete-cases",
            "no_reference_cases": "no-reference-cases",
        },  # схема передается в ответе, чтоб в случае ее смены (расширения полей) не катить фронт снова
        "data": {
            "items": items_list,
            "total": overall,
        }
    }
    return jsonify(answer), 200


@argus_api.route('/service/<string:service_id>/sbs_check/results/<int:result_id>')
def get_current_sbs_result(service_id, result_id):
    ensure_permissions_on_service(service_id, PermissionKind.SBS_RESULTS_SEE)
    result = SBSRuns.query.get_or_404(result_id)
    answer = result.as_dict()
    result = SBSResults.query.get_or_404(result_id)
    answer.update(result.as_dict())

    answer['owner'] = find_user_name(answer['owner'])

    return jsonify(answer), 200


@argus_api.route('/sbs_check/results', methods=['POST'])
def save_sbs_result():
    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=["AAB_SANDBOX_MONITORING_TVM_ID",
                                                   "AAB_CRYPROX_TVM_CLIENT_ID"])  # argus permissions
    results = Schema({
        Required('status'): Any(*SbSCheckStatus.all()),
        Required('start_time'): validate_ts,
        Required('end_time'): validate_ts,
        Required('sandbox_id'): Coerce(int),
        Required('cases'): Coerce(list),
        Required('filters_lists'): Coerce(list)
    })(request.get_json(force=True))
    sbs_run = SBSRuns.query.filter_by(sandbox_id=results['sandbox_id']).first_or_404()
    result_id = sbs_run.id
    sbs_run.status = results["status"]

    datetime_format = '%Y-%m-%dT%H:%M:%S'
    result = SBSResults(
        id=result_id,
        start_time=datetime.strptime(results["start_time"], datetime_format),
        end_time=datetime.strptime(results["end_time"], datetime_format),
        cases=results["cases"],
        filters_lists=results["filters_lists"],
    )
    db.session.add(result)
    db.session.commit()

    return "{}", 201


@argus_api.route('/sbs_check/runs')
def get_argus_active_runs():
    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=["AAB_SANDBOX_MONITORING_TVM_ID",
                                                   "AAB_CRYPROX_TVM_CLIENT_ID"])  # argus permissions
    query = SBSRuns.query.filter(
        (SBSRuns.status == SbSCheckStatus.NEW) | (SBSRuns.status == SbSCheckStatus.IN_PROGRESS)
    ).all()
    items = defaultdict(list)
    for sbs_run in query:
        items[sbs_run.status].append(dict(id=sbs_run.id, start_time=sbs_run.date, sandbox_id=sbs_run.sandbox_id))

    return jsonify(dict(items=items, total=len(query))), 200


@argus_api.route('/sbs_check/results/verdict_cases', methods=['GET'])
def get_adblock_cases_for_verdict():
    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=["AAB_SANDBOX_MONITORING_TVM_ID",
                                                   "AAB_CRYPROX_TVM_CLIENT_ID"])  # argus permissions
    current_time = datetime.now().replace(hour=0, minute=0, second=0, microsecond=0)
    raw_query = text("""
    WITH last_sbs_results AS (
        SELECT * FROM sbs_results
        WHERE start_time > '{START_TIME}'
    )
    SELECT
        t1.id,
        t1.request_id,
        t1.elastic_count,
        t1.reference_request_id,
        t1.start_time,
        t2.reference_elastic_count,
        t2.reference_start_time
    FROM (
         SELECT
                id,
                obj->'headers'->>'x-aab-requestid' AS request_id,
                obj->'logs'->>'elastic-count' as elastic_count,
                obj->>'reference_case_id' as reference_request_id,
                obj->>'start' as start_time
         FROM last_sbs_results, jsonb_array_elements(last_sbs_results.cases) AS obj
         WHERE
                obj->>'has_problem' = 'new'
                AND obj->'logs'->'elastic-count'->>'status' = 'success'
    ) as t1

    LEFT JOIN (
        SELECT
            obj->'headers'->>'x-aab-requestid' AS request_id,
            obj->'logs'->>'elastic-count' as reference_elastic_count,
            obj->>'start' as reference_start_time
        FROM last_sbs_results, jsonb_array_elements(last_sbs_results.cases) AS obj
        WHERE
            obj->'logs'->'elastic-count'->>'status' = 'success'
    ) as t2

    ON t1.reference_request_id = t2.request_id;
    """.format(START_TIME=(current_time - timedelta(days=2)).isoformat()))

    result_query = db.engine.execute(raw_query)
    adblock_cases = [dict(
        id=item.id,
        request_id=item.request_id,
        elastic_count=item.elastic_count,
        reference_request_id=item.reference_request_id,
        start_time=item.start_time,
        reference_elastic_count=item.reference_elastic_count,
        reference_start_time=item.reference_start_time,
    ) for item in result_query]
    return jsonify(data=adblock_cases, items=len(adblock_cases)), 200


@argus_api.route('/sbs_check/results/logs', methods=['GET'])
def get_cases_for_update_logs():
    """
    :return: [
        {
            'id': *SBS_Result_Id*,
            'request_id': *X-AAB-RequestId*,  # key for some case
            'adb_bits': *adb_bits*,
            'logs': *dict_logs*
        }, ...
    ]
    """
    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=["AAB_SANDBOX_MONITORING_TVM_ID",
                                                   "AAB_CRYPROX_TVM_CLIENT_ID"])  # argus permissions

    raw_query = text("""
        SELECT
            id,
            obj->'headers'->>'x-aab-requestid' AS request_id,
            obj->>'adb_bits' AS adb_bits,
            obj->>'start' AS start_time,
            obj->>'logs' AS logs
        FROM sbs_results, jsonb_array_elements(sbs_results.cases) AS obj
        WHERE
            obj->'logs'->'elastic-count'->>'status' IN ('{status_new}', '{status_wait}')
            OR obj->'logs'->'elastic-auction'->>'status' IN ('{status_new}', '{status_wait}');
    """.format(status_new=ArgusLogStatus.NEW, status_wait=ArgusLogStatus.WAIT_CONFIRMATION))
    results = db.engine.execute(raw_query)
    cases = []
    for item in results:
        cases.append(dict(
            id=item.id,
            request_id=item.request_id,
            adb_bits=item.adb_bits,
            logs=item.logs,
            start_time=item.start_time,
        ))
    return jsonify(cases), 200


# todo change request url
@argus_api.route('/sbs_check/results/logs', methods=['POST'])
def post_case_updates():
    """
    :input: { *SBS_Result_Id*: [{'request_id': *request_id*, 'logs': *logs* }, ...] }
    """
    ensure_tvm_ticket_is_ok(CONTEXT, allowed_apps=["AAB_SANDBOX_MONITORING_TVM_ID",
                                                   "AAB_CRYPROX_TVM_CLIENT_ID"])  # argus permissions
    logs_info = request.get_json(force=True)

    sbs_results = db.session.query(SBSResults).filter(SBSResults.id.in_(logs_info.keys())).all()
    for sbs_result in sbs_results:
        updated_cases = deepcopy(sbs_result.cases)
        for logged_case in logs_info[str(sbs_result.id)]:
            for item in updated_cases:
                if item["headers"]["x-aab-requestid"] == logged_case["request_id"]:
                    if "logs" in logged_case:
                        item["logs"] = logged_case["logs"]
                    if "has_problem" in logged_case:
                        item["has_problem"] = logged_case["has_problem"]
                    break
        sbs_result.cases = updated_cases
    db.session.commit()
    return "{}", 201


@argus_api.route('/sbs_check/results/<int:result_id>/logs/<string:logs_type>/id/<string:request_id>', methods=['GET'])
def get_rows_logs(result_id, logs_type, request_id):
    ensure_user_admin()

    try:
        logs_list = CONTEXT.argus_client.get_logs_list_from_s3(result_id, logs_type, request_id)
    except requests.exceptions.ConnectionError as e:
        return jsonify(msg="Can not connect to argus s3", log=str(e)), 408
    except Exception as e:
        return jsonify(msg=str(e)), 408

    schema = {}
    for item in logs_list:
        for name in item.keys():
            if name not in schema:
                schema[name] = name.replace("_", "-")

    return jsonify({
        "data": {
            "items": logs_list,
            "total": len(logs_list)
        },
        "schema": schema
    }), 200
