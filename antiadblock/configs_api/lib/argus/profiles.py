# coding=utf-8
from datetime import datetime

import re
import flask
from flask import Blueprint, jsonify, request
from voluptuous import Schema, Optional, Required, Coerce, All, Any, Invalid, Boolean, Length
from sqlalchemy import asc, desc, or_, text, and_

from antiadblock.libs.utils.utils import parse_and_validate_cookies

from antiadblock.configs_api.lib.jsonlogging import logging_context
from antiadblock.configs_api.lib.audit.audit import log_action

from antiadblock.configs_api.lib.argus.argus_api import basestring_encode, _get_last_sbs_profile

from antiadblock.configs_api.lib.auth.permissions import ensure_permissions_on_service, check_permissions_user_or_service
from antiadblock.configs_api.lib.auth.permission_models import PermissionKind
from antiadblock.configs_api.lib.validation.template import validate_url

from antiadblock.configs_api.lib.utils import check_args

from antiadblock.configs_api.lib.db import db, SBSProfiles, Service
from antiadblock.configs_api.lib.models import AuditAction

from antiadblock.configs_api.lib.service_api.api import basestring_strip

argus_profiles = Blueprint('argus_profiles', __name__)


@argus_profiles.route('/service/<string:service_id>/sbs_check/profile', methods=["GET"])
def get_current_sbs_profile(service_id):
    ensure_permissions_on_service(service_id, PermissionKind.SBS_PROFILE_SEE)
    args = check_args({Optional("tag", default="default"): All(basestring, basestring_strip)})
    tag = args.get("tag", "default")
    profile = _get_last_sbs_profile(service_id=service_id, empty_profile=True, tag=tag)
    return jsonify(profile), 200


@argus_profiles.route('/service/<string:service_id>/sbs_check/profile', methods=['POST'])
def create_sbs_profile(service_id):
    user_id = check_permissions_user_or_service(
        service_id, PermissionKind.SBS_PROFILE_UPDATE,
        allowed_service_apps=["AAB_SANDBOX_MONITORING_TVM_ID", "AAB_CRYPROX_TVM_CLIENT_ID"])

    def validate_cookies(cookies):
        try:
            parse_and_validate_cookies(cookies)
        except Exception as e:
            raise Invalid(str(e))
        return cookies

    def validate_url_settings(url_settings):
        if not url_settings:
            raise Invalid(u"List of url_settings is empty")
        for index, item in enumerate(url_settings):
            try:
                Schema({
                    Required("url"): validate_url,
                    Optional("selectors", default=[]): list,
                    Optional("wait_sec", default=0): int,
                    Optional("scroll_count", default=0): int,
                })(item)
            except Invalid as e:
                raise Invalid(message=str(e), path=[index])
        return url_settings

    def validate_general_settings(general_settings):
        return Schema({
            Optional("cookies", default=""): All(basestring, basestring_encode, validate_cookies),
            Optional("selectors", default=[]): list,
            Optional("headers", default={}): dict,
            Optional("filters_list", default=[]): list,
            Optional("wait_sec", default=0): int,
            Optional("scroll_count", default=0): int,
        })(general_settings)

    def validate_data(data_yaml):
        return Schema({
            Required("url_settings"): All(list, validate_url_settings),
            Optional("general_settings", default={}): All(dict, validate_general_settings)
        })(data_yaml)

    def validate_tag_name(tag):
        if re.match(r"^[a-z0-9.]+$", tag) is None:
            raise Invalid(u"Недопустимое имя тэга.\n"
                          u" Разрешены только буквы латинского алфавита в нижнем регистре, цифры и точки")
        return tag

    args = Schema({
        Required("data"): All(dict, validate_data),
        Optional("tag", default="default"): All(basestring,
                                                basestring_strip,
                                                validate_tag_name,
                                                Length(min=1, msg=u"Имя тэга не может быть пустым"),
                                                Length(min=3, msg=u"Имя тэга не может содержать менее 3 символов"))
    })(request.get_json(force=True))

    Service.query.get_or_404(service_id)

    with logging_context(service_id=service_id, user_id=user_id):
        profile = SBSProfiles(
            date=datetime.utcnow(),
            service_id=service_id,
            url_settings=args['data']['url_settings'],
            general_settings=args['data']['general_settings'],
            tag=args["tag"],
            is_archived=False,
        )
        old_profile_id = _get_last_sbs_profile(service_id, tag=args["tag"], empty_profile=True)["id"]
        profiles = SBSProfiles.query.filter_by(service_id=service_id, tag=args["tag"], is_archived=False).all()
        for item in profiles:
            item.is_archived = True
        db.session.add(profile)
        new_profile_id = _get_last_sbs_profile(service_id, tag=args["tag"])["id"]
        log_action(action=AuditAction.ARGUS_PROFILE_UPDATE, service_id=service_id,
                   user_id=user_id, new_profile_id=new_profile_id, old_profile_id=old_profile_id, label_id=service_id)
        db.session.commit()
        flask.current_app.logger.info(u"Created new sbs profile in service {}.".format(service_id),
                                      extra=dict(new_profile_id=new_profile_id, old_profile_id=old_profile_id))
    return "{}", 201


@argus_profiles.route('/service/<string:service_id>/sbs_check/profile/<int:profile_id>', methods=['GET'])
def get_profile_by_id(service_id, profile_id):
    check_permissions_user_or_service(service_id, PermissionKind.SBS_PROFILE_SEE, allowed_service_apps=[
        "AAB_SANDBOX_MONITORING_TVM_ID", "AAB_CRYPROX_TVM_CLIENT_ID"])
    profile = SBSProfiles.query.filter_by(id=profile_id, service_id=service_id).first_or_404()
    return jsonify(profile.as_dict()), 200


@argus_profiles.route('/service/<string:service_id>/sbs_check/tags', methods=['GET'])
def get_tags(service_id):
    ensure_permissions_on_service(service_id, PermissionKind.SBS_PROFILE_SEE)
    query = db.session.query(SBSProfiles.tag.distinct().label("tag")).filter(and_(SBSProfiles.service_id == service_id,
                                                                                  SBSProfiles.is_archived == False))
    tags = [row.tag for row in query.all()]
    return jsonify(dict(data=tags, items=len(tags))), 200


@argus_profiles.route('/service/<string:service_id>/sbs_check/tag/<string:tag_name>', methods=['GET'])
def get_profile_ids_by_tag(service_id, tag_name):
    ensure_permissions_on_service(service_id, PermissionKind.SBS_PROFILE_SEE)
    args = check_args({Optional("offset", default=0): Coerce(int),
                       Optional("limit", default=20): Coerce(int),
                       Optional("show_archived", default=False): Boolean()})
    query = SBSProfiles.query.filter_by(service_id=service_id, tag=tag_name)
    if not args["show_archived"]:
        query = query.filter_by(is_archived=False)

    overall = query.count()
    profiles = query.order_by(SBSProfiles.id.desc()).limit(args["limit"]).offset(args["offset"]).all()

    profile_ids = [item.id for item in profiles]
    return jsonify(dict(data=profile_ids, items=overall)), 200


@argus_profiles.route('/service/<string:service_id>/sbs_check/tag/<string:tag_name>', methods=['PATCH'])
def archive_tag(service_id, tag_name):
    check_permissions_user_or_service(service_id, PermissionKind.SBS_PROFILE_SEE, allowed_service_apps=[
        "AAB_SANDBOX_MONITORING_TVM_ID", "AAB_CRYPROX_TVM_CLIENT_ID"])
    profiles = SBSProfiles.query.filter_by(service_id=service_id, tag=tag_name, is_archived=False).all()
    for item in profiles:
        item.is_archived = True
    db.session.commit()
    return "{}", 201
