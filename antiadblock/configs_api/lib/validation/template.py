# coding=utf-8
import sys
import re2
from datetime import datetime

from voluptuous import All, Required, Optional, Schema, Any, Length, Invalid, ALLOW_EXTRA, Equal, Range, Url

from antiadblock.configs_api.lib.context import CONTEXT
from antiadblock.configs_api.lib.auth.nodes import ROOT
from antiadblock.configs_api.lib.utils import ListableEnum
from antiadblock.configs_api.lib.const import DEFAULT_COOKIE, EXPERIMENT_DATETIME_FORMAT, DEFAULT_YAUID_COOKIE, \
    SUPPORTED_YAUID_COOKIES, ALLOWED_CSP
from antiadblock.configs_api.lib.validation.validation_utils import Ordered
from antiadblock.configs_api.lib.auth.permission_models import PermissionKind
from antiadblock.configs_api.lib.auth.permissions import get_user_permissions

re2.set_fallback_notification(re2.FALLBACK_EXCEPTION)


class GroupField(object):
    def __init__(self, name, fields):
        self.name = name
        self.fields = list(fields)
        # see for title and hints https://tanker.yandex-team.ru/?project=antiadb&branch=master&keyset=config-editor-group
        self.title = name.replace('_', '-').lower() + "-title"
        self.hint = name.replace('_', '-').lower() + "-hint"


class Field(object):
    def __init__(self,
                 name,
                 type_schema,
                 default,
                 validation,
                 required=False,
                 see_permission=PermissionKind.SERVICE_SEE,
                 change_permission=PermissionKind.CONFIG_CREATE):
        self.default = default
        self.required = required
        self.change_permission = change_permission
        self.see_permission = see_permission
        self.validation = validation
        self.name = name
        children = type_schema.get("children")
        if children:
            if "placeholder" not in children:
                type_schema["children"]["placeholder"] = name.replace('_', '-').lower() + "-placeholder"
        elif "placeholder" not in type_schema:
            type_schema["placeholder"] = name.replace('_', '-').lower() + "-placeholder"

        for label in ["hint", "title"]:
            if label not in type_schema:
                type_schema[label] = name.replace('_', '-').lower() + "-" + label
        self.type_schema = type_schema


class Template(object):
    def __init__(self, groups):
        """
        :type groups: list [GroupField]
        """
        self.fields = {field.name: field for group in groups for field in group.fields}
        self.groups = list(groups)

    def get_type_validation_schema(self):
        """
        checks types for config fields
        :return:
        """
        schema_dict = {}
        for field in self.fields.itervalues():
            if field.required:
                schema_dict[Required(field.name, msg=u"{} обязательное поле".format(field.name))] = field.validation
            else:
                schema_dict[Optional(field.name)] = field.validation
        return Schema(schema_dict)

    def get_type_validation_schema_without_required(self):
        """
        checks types for config fields
        :return:
        """
        return Schema({Optional(field.name): field.validation for field in self.fields.itervalues()})

    def get_immutable_fields_validation_schema(self, service_id, parent_config_data):
        """
        checks that user didn't change hidden and protected fields
        :param service_id:
        :param parent_config_data:
        :return:
        """
        node = ROOT.SERVICES[service_id] if service_id else ROOT
        schema_dict = {}
        for field in self.fields.itervalues():
            previous_value = parent_config_data.get(field.name)
            if not (get_user_permissions(CONTEXT.auth.get_user_id())
                    .check_permission(node, field.change_permission)):

                immutable_validator = Equal(previous_value, u"Недостаточно прав, чтобы изменить поле {field_name}"
                                            .format(field_name=field.name))
                # if field is required it should be checked in 'get_type_validation_schema'
                schema_dict[Optional(field.name)] = immutable_validator
        # empty dict is not working with ALLOW_EXTRA
        if len(schema_dict) == 0:
            return lambda v: v
        return Schema(schema_dict, extra=ALLOW_EXTRA)

    def get_frontend_schema(self, service_id=None):
        node = ROOT.SERVICES[service_id] if service_id else ROOT
        schema_list = []
        for group in self.groups:
            schema_group = {'group_name': group.name, 'title': group.title, 'hint': group.hint, 'items': []}
            for field in group.fields:
                if not get_user_permissions(CONTEXT.auth.get_user_id()).check_permission(node, field.see_permission):
                    continue
                # See for placeholders: https://tanker.yandex-team.ru/?project=antiadb&branch=master&keyset=placeholders
                # See for hints: https://tanker.yandex-team.ru/?project=antiadb&branch=master&keyset=hints
                field_descriptor = dict(type_schema=field.type_schema,
                                        default=field.default,
                                        name=field.name)
                if field.required:
                    field_descriptor['required'] = True
                if not get_user_permissions(CONTEXT.auth.get_user_id()).check_permission(node, field.change_permission):
                    field_descriptor['read_only'] = True
                schema_group['items'].append({'key': field_descriptor})
            schema_list.append(schema_group)
        return schema_list

    def filter_hidden_fields(self, service_id, config_data):
        node = ROOT.SERVICES[service_id] if service_id else ROOT
        filtered_data = {}
        for key, value in config_data.iteritems():
            if key not in self.fields:
                raise ValueError(u"Неизвестное поле {}".format(key))
            if not get_user_permissions(CONTEXT.auth.get_user_id()).check_permission(node, self.fields[key].see_permission):
                continue
            filtered_data[key] = value
        return filtered_data

    def fill_defaults(self, config_data):
        for k, v in self.fields.iteritems():
            if k not in config_data and v.default is not None:
                config_data[k] = v.default
        return config_data

    def to_ranged_list(self, config_data):
        unsorted = [dict(name=k, data=v) for k, v in config_data.iteritems()]
        return sorted(unsorted, key=lambda f: self.fields.get(f['name'].order))


# Do not forget to add translation keys to tanker! Aka 'encryption-steps-advertising-crypt-hint', 'encryption-steps-advertising-crypt-title'
class EncryptionSteps(ListableEnum):
    PCODE_REPLACE = 0
    ADVERTISING_CRYPT = 1
    PARTNER_CONTENT_URL_CRYPT = 2
    PARTNER_CONTENT_CLASS_CRYPT = 3
    TAGS_CRYPT = 4
    LOADED_JS_CRYPT = 5
    INLINE_JS_CRYPT = 6
    CRYPT_RELATIVE_URLS_AUTOMATICALLY = 7


# https://a.yandex-team.ru/arc/trunk/arcadia/yabs/server/libs/enums/ad_type.h?rev=7144456#L10
class DisabledAdType(ListableEnum):
    (TEXT,
     MEDIA,
     MEDIA_PERF,
     VIDEO,
     VIDEO_PERF,
     VIDEO_MOTION,
     AUDIO,
     VIDEO_NONSKIPABLE) = range(8)


# Do not forget to add translation keys to tanker! Aka 'cookiematching-image-hint', 'cookiematching-image-title'
class CookieMatching(ListableEnum):
    IMAGE = 1
    REFRESH = 2
    CRYPTED_UID = 4

    @staticmethod
    def order():
        return [CookieMatching.CRYPTED_UID, CookieMatching.IMAGE, CookieMatching.REFRESH]


# Do not forget to add translation keys to tanker! Aka 'cookiematching-image-hint', 'cookiematching-image-title'
class AdSystems(ListableEnum):
    BK = 1
    ADFOX = 2
    # AWAPS = 3 deprecated
    RAMBLER_SSP = 4


# Do not forget to add translation keys to tanker! Aka 'cookiematching-image-hint', 'cookiematching-image-title'
class DetectCookie(ListableEnum):
    CURRENT = 0
    CHILDREN = 1
    LIST = 2


# Do not forget to add translation keys to tanker! Aka 'crypted-url-mixing-template-hint', 'crypted-url-mixing-template-title'
class CryptedUrlMixingTemplate(ListableEnum):
    # symbols allowed in templates: /?&.*![]~$
    MANY_SLASHES = [['/', 3000]]
    QUERY_STRING = [['/', 2], ['?.', 1], ['=&', 3000]]
    ONE_SLASH = [['/', 1]]


# Do not forget to add translation keys to tanker! Aka 'iframe-detect-link-hint', 'iframe-detect-link-title'
class DetectIframeLink(ListableEnum):
    # symbols allowed in templates: /?&.*![]~$
    MDS = "https://storage.mds.yandex.net/get-get-frame-content/1731675/frame.html"
    YASTATIC = "https://yastatic.net/aab-pub/frame.html"


# Do not forget to add translation keys to tanker! Aka 'inline-js-script-position-{value}-placeholder'
class InjectInlineJSPosition(ListableEnum):
    HEAD_BEGINNING = 0
    HEAD_END = 1


# Do not forget to add translation keys to tanker! Aka 'seed-change-period-template-hint', 'seed-change-period-template-title'
class SeedChangePeriod(ListableEnum):
    ONE_HOUR = 1
    SIX_HOURS = 6
    TWELVE_HOURS = 12
    TWENTY_FOUR_HOURS = 24

    @staticmethod
    def order():
        return [SeedChangePeriod.ONE_HOUR, SeedChangePeriod.SIX_HOURS, SeedChangePeriod.TWELVE_HOURS, SeedChangePeriod.TWENTY_FOUR_HOURS]


class CookielessDomain(ListableEnum):
    NAYDEX = 'naydex.net'
    CLSTORAGE = 'clstorage.net'
    STATIC_STORAGE = 'static-storage.net'
    CDNCLAB = 'cdnclab.net'
    YASTATIC = 'yastatic.net'
    # YASTATIC_RU = 'yastatic-net.ru'
    # NAYDEX_RU = 'naydex-net.ru'
    # CLSTORAGE_RU = 'clstorage-net.ru'
    # STATIC_STORAGE_RU = 'static-storage-net.ru'
    # CDNCLAB_RU = 'cdnclab-net.ru'


class Experiments(ListableEnum):
    NONE = 0
    BYPASS = 1
    FORCECRY = 2
    NOT_BYPASS_BYPASS_UIDS = 3


# Do not forget to add translation keys to tanker! Aka 'dc-man-hint', 'dc-man-title'
class DataCenters(ListableEnum):
    MAN = 1
    SAS = 2
    MYT = 3
    VLA = 4
    IVA = 5


# Do not forget to add translation keys to tanker! Aka 'device-desktop-hint', 'device-desktop-title'
class UserDevice(ListableEnum):
    DESKTOP = 0
    MOBILE = 1


DEFAULT_EXPERIMENT = {
    'EXPERIMENT_TYPE': Experiments.NONE,
    'EXPERIMENT_PERCENT': None,
    'EXPERIMENT_START': None,
    'EXPERIMENT_DURATION': None,
    'EXPERIMENT_DAYS': [],
    'EXPERIMENT_DEVICE': UserDevice.all(),
}


TOKENS_VALIDATOR = All([basestring], Length(min=1, msg=u"Должен быть задан хотя бы 1 токен"))
TEST_DATA_VALIDATOR = Schema({'valid_domains': [basestring],
                              'invalid_domains': [basestring],
                              'paths': Schema({'valid': [basestring],
                                               'invalid': [basestring],
                                               })})


def compilable_re(re):
    re = Schema(basestring)(re)
    try:
        re2.compile(re)
        return re
    except Exception:
        raise Invalid(u"Регулярное выражение не может быть скомпилировано")


def validate_ts(ts):
    ts = Schema(basestring)(ts)
    try:
        datetime.strptime(ts, EXPERIMENT_DATETIME_FORMAT)
        return ts
    except Exception:
        raise Invalid(u"Некорректный формат времени")


def validate_url(url):
    url = Schema(basestring)(url)
    if url:
        try:
            url = Schema(Url())(url)
        except Exception as e:
            raise Invalid(str(e))
    return url


def validate_unique_list(item_list):
    checked = dict()
    list_of_doubles = list()
    for item in item_list:
        if item in checked:
            list_of_doubles.append(item)
        checked[item] = True
    if list_of_doubles:
        list_of_doubles = sorted(set(list_of_doubles))
        raise Invalid(u"Элементы {} повторяются.".format(", ".join(list_of_doubles)))
    return item_list


def validate_patched_csp_dict(patched_csp):
    patched_csp = Schema({basestring: [basestring]})(patched_csp)
    bad_csp = []
    empty_csp = []
    for csp_key, csp_value in patched_csp.iteritems():
        if csp_key not in ALLOWED_CSP:
            bad_csp.append(csp_key)
        elif not csp_value:
            empty_csp.append(csp_key)
    if bad_csp:
        raise Invalid(u"Недопустимые политики: {}.".format(", ".join(bad_csp)))
    if empty_csp:
        raise Invalid(u"Пустые значения для политик: {}.".format(", ".join(empty_csp)))
    return patched_csp


TEMPLATE = Template([
    # SECURITY fields section
    GroupField(name='SECURITY', fields=[
        Field(name='PROXY_URL_RE',
              default=[],
              validation=All([compilable_re],
                             Length(min=1, msg=u"Поле должно содержать хотя бы одно регулярное выражение")),
              required=True,
              type_schema=dict(type="array", children=dict(type="regexp", placeholder="regexp-placeholder"))),
        Field(name='EXCLUDE_COOKIE_FORWARD',
              default=[DEFAULT_COOKIE],
              validation=[All(basestring, Length(min=1, msg=u"Поле не может быть пустым"))],
              type_schema=dict(type="tags", children=dict(type="string"))),
        Field(name='CURRENT_COOKIE',
              default=DEFAULT_COOKIE,
              validation=All(basestring, Length(min=1, msg=u"Поле не может быть пустым")),
              type_schema=dict(type="string")),
        Field(name='DEPRECATED_COOKIES',
              default=[],
              validation=[basestring],
              type_schema=dict(type="tags", children=dict(type="string"))),
        Field(name='WHITELIST_COOKIES',
              default=[DEFAULT_COOKIE],
              validation=[basestring],
              type_schema=dict(type="tags", children=dict(type="string"))),
        Field(name="CRYPTED_YAUID_COOKIE_NAME",
              default=DEFAULT_YAUID_COOKIE,
              validation=All(basestring, Length(min=1, msg=u"Поле не может быть пустым"),
                             Any(*SUPPORTED_YAUID_COOKIES)),
              type_schema=dict(type="string"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='PARTNER_TOKENS',
              default=[],
              validation=TOKENS_VALIDATOR,
              required=True,
              type_schema=dict(type="tokens", children=dict(type="string")),
              change_permission=PermissionKind.TOKEN_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='PUBLISHER_SECRET_KEY',
              default=None,
              validation=All(basestring, Length(min=1, msg=u"Поле не может быть пустым")),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="string")),
    ]),
    # ENCRYPTION fields section
    GroupField(name='ENCRYPTION', fields=[
        Field(name='CRYPT_BODY_RE',
              default=[],
              validation=[compilable_re],
              type_schema=dict(type="array",
                               children=dict(type="regexp", placeholder="regexp-placeholder"),
                               hint="crypt-body-re-crypt-hint",
                               title="crypt-body-re-crypt-title")),
        Field(name='REPLACE_BODY_RE',
              default={},
              validation={compilable_re: basestring},
              type_schema=dict(type="object",
                               key="string",
                               value="string",
                               hint="crypt-body-re-substitute-hint",
                               title="crypt-body-re-substitute-title",
                               placeholder=dict(key_placeholder="regexp-placeholder",
                                                value_placeholder="crypt-body-re-substitute-placeholder"))),
        Field(name='REPLACE_BODY_RE_PER_URL',
              default={},
              validation={compilable_re: {compilable_re: basestring}},
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="json")),
        Field(name='REPLACE_BODY_RE_EXCEPT_URL',
              default={},
              validation={compilable_re: {compilable_re: basestring}},
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="json")),
        Field(name='CRYPT_URL_RE',
              default=[],
              validation=[compilable_re],
              required=True,
              type_schema=dict(type="array", children=dict(type="regexp", placeholder="regexp-placeholder"))),
        Field(name='BYPASS_URL_RE',
              default=[],
              validation=[compilable_re],
              required=False,
              type_schema=dict(type="array", children=dict(type="regexp", placeholder="regexp-placeholder"))),
        Field(name='CRYPT_RELATIVE_URL_RE',
              default=[],
              validation=[compilable_re],
              type_schema=dict(type="array", children=dict(type="string", placeholder="regexp-placeholder"))),
        Field(name='CRYPT_IN_LOWERCASE',
              default=False,
              validation=bool,
              type_schema=dict(type="bool")),
    ]),
    # ENCRYPTION_SETTINGS fields section
    GroupField(name='ENCRYPTION_SETTINGS', fields=[
        Field(name='DEVICE_TYPE',
              default=None,
              validation=Any(*UserDevice.all()),
              type_schema=dict(type="select", values={"device-" + k.lower().replace('_', '-') + "-placeholder": v for k, v in UserDevice.key_values().iteritems()}),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='ENCRYPTION_STEPS',
              default=[EncryptionSteps.PCODE_REPLACE],
              required=True,
              validation=[Any(*EncryptionSteps.all())],
              type_schema=dict(type="multi_checkbox",
                               values=[dict(value=v,
                                            hint="encryption-steps-" + k.lower().replace('_', '-') + "-hint",
                                            title="encryption-steps-" + k.lower().replace('_', '-') + "-title") for k, v in EncryptionSteps.key_values().iteritems()]),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='SEED_CHANGE_PERIOD',
              default=SeedChangePeriod.SIX_HOURS,
              validation=Any(*SeedChangePeriod.all()),
              # TODO: Make some sorted dict/list here
              type_schema=dict(type="select", values={"seed-change-period-" + k.lower().replace('_', '-') + "-placeholder": v for k, v in SeedChangePeriod.key_values().iteritems()}),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='SEED_CHANGE_TIME_SHIFT_MINUTES',
              default=0,
              validation=All(int, Range(min=-1440, max=1440, msg=u"Поле должно содержать целое число от -1440 до 1440")),
              type_schema=dict(type="number"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='CRYPT_SECRET_KEY',
              default=None,
              validation=All(basestring, Length(min=1, msg=u"Поле не может быть пустым")),
              required=True,
              type_schema=dict(type="string"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='COOKIE_CRYPT_KEY',
              default="1sBGV1c978bb0366",
              validation=All(basestring, Length(min=16, msg=u"Поле должно содержать не менее 16 символов")),
              type_schema=dict(type="string"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='AD_SYSTEMS',
              default=[AdSystems.BK],  # https://st.yandex-team.ru/ANTIADB-1375
              validation=[Any(*AdSystems.all())],
              type_schema=dict(type="multi_checkbox",
                               values=[dict(value=v,
                                            title="ad-systems-" + k.lower().replace('_', '-') + "-title",
                                            hint="ad-systems-" + k.lower().replace('_', '-') + "-hint") for k, v in AdSystems.key_values().items()])),
        Field(name='CRYPT_BS_COUNT_URL',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='DISABLED_AD_TYPES',
              default=[DisabledAdType.VIDEO_PERF, DisabledAdType.VIDEO_MOTION, DisabledAdType.VIDEO],
              validation=[Any(*DisabledAdType.all())],
              type_schema=dict(type="multi_checkbox",
                               values=[dict(value=v,
                                            hint="disable-ad-types-" + k.lower().replace('_', '-') + "-hint",
                                            title="disable-ad-types-" + k.lower().replace('_', '-') + "-title") for k, v
                                       in DisabledAdType.key_values().iteritems()]),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='DISABLE_TGA_WITH_CREATIVES',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='DISABLE_ADB_ENABLED',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='REPLACE_ADB_FUNCTIONS',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='CRYPTED_URL_MIN_LENGTH',
              default=0,
              validation=int,
              type_schema=dict(type="number"),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='CRYPTED_URL_MIXING_TEMPLATE',
              default=CryptedUrlMixingTemplate.MANY_SLASHES,
              validation=Any(*[Equal(v) for v in CryptedUrlMixingTemplate.all()]),
              type_schema=dict(type="select", values={"crypted-url-mixing-template-" + k.lower().replace('_', '-') + "-placeholder": v for k, v in CryptedUrlMixingTemplate.key_values().iteritems()}),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='IMAGE_URLS_CRYPTING_PROBABILITY',
              default=100,
              validation=All(int, Range(min=0, max=100, msg=u"Поле должно содержать целое число от 0 до 100")),
              type_schema=dict(type="number"),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='HOSTNAME_MAPPING',
              default={},
              validation={basestring: basestring},
              type_schema=dict(type="object",
                               key="string",
                               value="string",
                               placeholder=dict(key_placeholder="hostname-mapping-key-placeholder",
                                                value_placeholder="hostname-mapping-value-placeholder")),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='CRYPTED_HOST',
              default=None,
              validation=All(basestring, Length(min=1, msg=u"Поле не может быть пустым")),
              type_schema=dict(type="string"),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='CRYPT_URL_PREFFIX',
              default="/",
              validation=All(basestring, Length(min=1, msg=u"Поле не может быть пустым")),
              type_schema=dict(type="string"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='CRYPT_URL_OLD_PREFFIXES',
              default=[],
              validation=validate_unique_list,
              type_schema=dict(type="array", children=dict(type="string", placeholder="crypt-url-preffix-placeholder")),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='CRYPT_URL_RANDOM_PREFFIXES',
              default=[],
              validation=validate_unique_list,
              type_schema=dict(type="array", children=dict(type="string", placeholder="crypt-url-preffix-placeholder")),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='ENCRYPT_TO_THE_TWO_DOMAINS',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='PARTNER_COOKIELESS_DOMAIN',
              default=CookielessDomain.NAYDEX,
              validation=Any(*CookielessDomain.all()),
              type_schema=dict(type="select", values={v: v for v in CookielessDomain.key_values().values()}),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='PARTNER_TO_COOKIELESS_HOST_URLS_RE',
              default=[],
              validation=[compilable_re],
              type_schema=dict(type="array", children=dict(type="string", placeholder="regexp-placeholder")),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='AVATARS_NOT_TO_COOKIELESS',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
    ]),
    # FEATURES fields section
    GroupField(name='FEATURES', fields=[
        Field(name='INVERTED_COOKIE_ENABLED',
              default=False,
              validation=bool,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="bool")),
        Field(name='DIV_SHIELD_ENABLE',
              default=False,
              validation=bool,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="bool")),
        Field(name='RTB_AUCTION_VIA_SCRIPT',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='HIDE_META_ARGS_ENABLED',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='HIDE_META_ARGS_HEADER_MAX_SIZE',
              default=6144,
              validation=int,
              type_schema=dict(type="number"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='REMOVE_SCRIPTS_AFTER_RUN',
              default=False,
              validation=bool,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="bool")),
        Field(name='ADD_NONCE',
              default=False,
              validation=bool,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="bool")),
        Field(name='REMOVE_ATTRIBUTE_ID',
              default=False,
              validation=bool,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="bool")),
        Field(name='UPDATE_RESPONSE_HEADERS_VALUES',
              default={},
              validation={compilable_re: {basestring: basestring}},
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="json")),
        Field(name='UPDATE_REQUEST_HEADER_VALUES',
              default={},
              validation={compilable_re: {basestring: basestring}},
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="json")),
        Field(name='DISABLE_SHADOW_DOM',
              default=False,
              validation=bool,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="bool")),
        Field(name='CRYPT_ENABLE_TRAILING_SLASH',
              default=False,
              validation=bool,
              type_schema=dict(type="bool",
                               placeholder=dict(placeholder_on="crypt-enable-trailing-slash-on-placeholder",
                                                placeholder_off="crypt-enable-trailing-slash-off-placeholder")),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='CRYPT_LINK_HEADER',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='HIDE_LINKS',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='VISIBILITY_PROTECTION_CLASS_RE',
              default=None,
              validation=compilable_re,
              type_schema=dict(type="string", placeholder="regexp-placeholder"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='XHR_PROTECT',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='BREAK_OBJECT_CURRENT_SCRIPT',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='LOADED_JS_PROTECT',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='REPLACE_SCRIPT_WITH_XHR_RE',
              default=[],
              validation=[compilable_re],
              type_schema=dict(type="array", children=dict(type="regexp", placeholder="regexp-placeholder")),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='REPLACE_RESOURCE_WITH_XHR_SYNC_RE',
              default=[],
              validation=[compilable_re],
              type_schema=dict(type="array", children=dict(type="regexp", placeholder="regexp-placeholder")),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='IMAGE_TO_IFRAME_URL_RE',
              default=[],
              validation=[compilable_re],
              type_schema=dict(type="array", children=dict(type="regexp",
                                                           placeholder="regexp-placeholder")),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='IMAGE_TO_IFRAME_URL_LENGTH',
              default=0,
              validation=int,
              type_schema=dict(type="number"),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='IMAGE_TO_IFRAME_URL_IS_RELATIVE',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='IMAGE_TO_IFRAME_CHANGING_PROBABILITY',
              default=100,
              validation=All(int, Range(min=0, max=100, msg=u"Поле должно содержать целое число от 0 до 100")),
              type_schema=dict(type="number"),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='USE_CACHE',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='NO_CACHE_URL_RE',
              default=[],
              validation=[compilable_re],
              required=False,
              type_schema=dict(type="array", children=dict(type="regexp", placeholder="regexp-placeholder")),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='BYPASS_BY_UIDS',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='COUNT_TO_XHR',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='INJECT_INLINE_JS',
              default=None,
              validation=basestring,
              type_schema=dict(type="detect_html"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='INJECT_INLINE_JS_POSITION',
              default=InjectInlineJSPosition.HEAD_END,
              validation=Any(*InjectInlineJSPosition.all()),
              type_schema=dict(type="select",
                               values={"inject-inline-js-position-" + k.lower().replace('_', '-') + "-placeholder": v
                                       for k, v in InjectInlineJSPosition.key_values().iteritems()}),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='BLOCK_TO_IFRAME_SELECTORS',
              default=[],
              validation=validate_unique_list,
              type_schema=dict(type="array", children=dict(type="string", placeholder="block-to-iframe-selectors-placeholder")),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='CSP_PATCH',
              default={},
              validation=validate_patched_csp_dict,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="json")),
        Field(name='ADDITIONAL_PCODE_PARAMS',
              default={},
              validation=dict,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="json")),
        Field(name='NETWORK_FAILS_RETRY_THRESHOLD',
              default=0,
              validation=All(int, Range(min=0, max=200, msg=u"Поле должно содержать целое число от 0 до 200")),
              type_schema=dict(type="number"),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
    ]),
    # DETECT_SETTINGS fields section
    GroupField(name='DETECT_SETTINGS', fields=[
        Field(name='DETECT_HTML',
              default=[],
              validation=[basestring],
              type_schema=dict(type="array", children=dict(type="detect_html"))),
        Field(name='DETECT_LINKS',
              default=[],
              validation=[Schema({
                  Required('src'): basestring,
                  Required('type'): basestring,
              })],
              type_schema=dict(type="array", children=dict(type="link", placeholder="link-placeholder"))),
        Field(name='DETECT_TRUSTED_LINKS',
              default=[],
              validation=[Schema({
                  Required('src'): basestring,
                  Required('type'): basestring,
              })],
              type_schema=dict(type="array", children=dict(type="link", placeholder="link-placeholder"))),
        Field(name='DETECT_IFRAME',
              default=[],
              validation=[Any(*DetectIframeLink.all())],
              type_schema=dict(type="multi_checkbox", values=[dict(value=v,
                                                                   title="detect-iframe-link-" + k.lower().replace('_', '-') + "-title",
                                                                   hint="detect-iframe-link-" + k.lower().replace('_', '-') + "-hint") for k, v in DetectIframeLink.key_values().items()])),
        Field(name='DETECT_COOKIE_TYPE',
              default=DetectCookie.CURRENT,
              validation=Any(*DetectCookie.all()),
              type_schema=dict(type="select", values={"detect-cookie-type-" + k.lower().replace('_', '-') + "-placeholder": v for k, v in DetectCookie.key_values().iteritems()}),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='DETECT_COOKIE_DOMAINS',
              default=[],
              validation=[basestring],
              type_schema=dict(type="array", children=dict(type="string")),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='DISABLE_DETECT',
              default=False,
              validation=bool,
              type_schema=dict(type="bool")),
        Field(name='NEW_DETECT_SCRIPT_URL',
              default="",
              validation=validate_url,
              type_schema=dict(type="string"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='NEW_DETECT_SCRIPT_DC',
              default=[],
              validation=[Any(*DataCenters.all())],
              type_schema=dict(type="multi_checkbox", values=[dict(value=v,
                                                                   title="dc-" + k.lower().replace('_', '-') + "-title",
                                                                   hint="dc-" + k.lower().replace('_', '-') + "-hint") for k, v in DataCenters.key_values().items()]),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='AUTO_SELECT_DETECT',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
    ]),
    # COOKIEMATCHING_SETTINGS fields section
    GroupField(name='COOKIEMATCHING_SETTINGS', fields=[
        Field(name='CM_TYPE',
              default=[CookieMatching.IMAGE, CookieMatching.REFRESH],
              validation=Ordered(CookieMatching.order(),
                                 msg=u"Элементы в списке типов кукиматчинга должны идти в порядке: {}"
                                 .format(CookieMatching.order())),
              type_schema=dict(type="multi_checkbox",
                               values=[dict(value=v,
                                            title="cookiematching-" + k.lower().replace('_', '-') + "-title",
                                            hint="cookiematching-" + k.lower().replace('_', '-') + "-hint") for k, v in
                                       sorted(CookieMatching.key_values().iteritems(), key=lambda v: CookieMatching.order().index(v[1]) if v[1] in CookieMatching.order() else sys.maxint)]),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='EXTUID_COOKIE_NAMES',
              default=['addruid'],
              validation=[All(basestring, Length(min=1, msg=u"Поле не может быть пустым"))],
              type_schema=dict(type="tags", children=dict(type="string")),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='EXTUID_TAG',
              default=None,
              validation=All(basestring, Length(min=1, msg=u"Поле не может быть пустым")),
              type_schema=dict(type="string"),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='CM_IMAGE_URL',
              default='https://statchecker.yandex.ru/mapuid/',
              validation=All(basestring, Length(min=1, msg=u"Поле не может быть пустым")),
              type_schema=dict(type="string",
                               placeholder="link-placeholder"),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='CM_REDIRECT_URL',
              default='//an.yandex.ru/mapuid/',
              validation=All(basestring, Length(min=1, msg=u"Поле не может быть пустым")),
              type_schema=dict(type="string",
                               placeholder="link-placeholder"),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
    ]),
    # INTERNAL_PARTNER_SETTINGS fields section
    GroupField(name='INTERNAL_PARTNER_SETTINGS', fields=[
        Field(name='INTERNAL',
              default=False,
              validation=bool,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="bool",
                               placeholder=dict(placeholder_on="internal-on-placeholder",
                                                placeholder_off="internal-off-placeholder"))),
        Field(name='SERVICE_SLB_URL_RE',
              default=[],
              validation=[compilable_re],
              type_schema=dict(type="array", children=dict(type="regexp", placeholder="regexp-placeholder")),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='PARTNER_BACKEND_URL_RE',
              default=[],
              validation=[compilable_re],
              type_schema=dict(type="array", children=dict(type="regexp", placeholder="regexp-placeholder")),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='BALANCERS_PROD',
              default=[],
              validation=[basestring],
              type_schema=dict(type="array", children=dict(type="string")),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
        Field(name='BALANCERS_TEST',
              default=[],
              validation=[basestring],
              type_schema=dict(type="array", children=dict(type="string")),
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE),
    ]),
    # REDIRECT_SETTINGS fields section
    GroupField(name='REDIRECT_SETTINGS', fields=[
        Field(name='FOLLOW_REDIRECT_URL_RE',
              default=[],
              validation=[compilable_re],
              type_schema=dict(type="array", children=dict(type="regexp", placeholder="regexp-placeholder"))),
        Field(name='CLIENT_REDIRECT_URL_RE',
              default=[],
              validation=[compilable_re],
              type_schema=dict(type="array", children=dict(type="regexp",
                                                           placeholder="regexp-placeholder"))),
        Field(name='ACCEL_REDIRECT_URL_RE',
              default=[],
              validation=[compilable_re],
              type_schema=dict(type="array", children=dict(type="regexp",
                                                           placeholder="regexp-placeholder"))),
        Field(name='PARTNER_ACCEL_REDIRECT_ENABLED',
              default=False,
              validation=bool,
              type_schema=dict(type="bool"),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='PROXY_ACCEL_REDIRECT_URL_RE',
              default=[],
              validation=[compilable_re],
              type_schema=dict(type="array", children=dict(type="regexp",
                                                           placeholder="regexp-placeholder")),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
    ]),
    # DEBUG fields section
    GroupField(name='DEBUG', fields=[
        Field(name='ADFOX_DEBUG',
              default=False,
              validation=bool,
              type_schema=dict(type="bool",
                               placeholder=dict(placeholder_on="adfox-debug-on-placeholder",
                                                placeholder_off="adfox-debug-off-placeholder")),
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE),
        Field(name='AIM_BANNER_ID_DEBUG_VALUE',
              default=None,
              validation=basestring,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="string")),
        Field(name='DEBUG_LOGGING_ENABLE',
              default=False,
              validation=bool,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="bool")),
        Field(name='TEST_DATA',
              default=None,
              validation=TEST_DATA_VALIDATOR,
              type_schema=dict(type="test_data",
                               hint=dict(valid_hint="test-data-hint",
                                         invalid_hint="test-data-hint",
                                         paths_valid_hint='test-data-paths-valid-hint',
                                         paths_invalid_hint='test-data-paths-invalid-hint'),
                               title=dict(valid_title="test-data-valid-title",
                                          invalid_title="test-data-invalid-title",
                                          paths_valid_title='test-data-paths-valid-title',
                                          paths_invalid_title='test-data-paths-invalid-title'),
                               placeholder=dict(valid_placeholder="test-data-placeholder",
                                                invalid_placeholder="test-data-placeholder",
                                                paths_valid_placeholder='test-data-paths-placeholder',
                                                paths_invalid_placeholder='test-data-paths-placeholder'))),
    ]),
    # EXPERIMENTS fields section
    GroupField(name='EXPERIMENTS', fields=[
        Field(name='EXPERIMENTS',
              default=[DEFAULT_EXPERIMENT],
              validation=list,
              change_permission=PermissionKind.HIDDEN_FIELDS_UPDATE,
              see_permission=PermissionKind.HIDDEN_FIELDS_SEE,
              type_schema=dict(type="json"))
    ]),
])
