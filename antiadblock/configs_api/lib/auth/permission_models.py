# coding=utf-8
from antiadblock.configs_api.lib.utils import ListableEnum


# TODO: think about move this to db
class PermissionKind(ListableEnum):
    SERVICE_CREATE = 'service_create'
    SERVICE_SEE = 'service_see'
    SERVICE_STATUS_SWITCH = 'service_status_switch'
    LABEL_CREATE = 'label_create'
    CHANGE_PARENT_LABEL = 'change_parent_label'
    PARENT_CONFIG_CREATE = 'parent_config_create'
    CONFIG_MARK_TEST = 'config_mark_test'
    CONFIG_MARK_ACTIVE = 'config_mark_active'
    CONFIG_MARK_EXPERIMENT = 'config_mark_experiment'
    CONFIG_CREATE = 'config_create'
    CONFIG_ARCHIVE = 'config_archive'
    CONFIG_MODERATE = 'config_moderate'
    HIDDEN_FIELDS_SEE = 'hidden_fields_see'
    HIDDEN_FIELDS_UPDATE = 'hidden_fields_update'
    TOKEN_UPDATE = 'token_update'
    AUTH_GRANT = 'auth_grant'
    SERVICE_COMMENT = 'service_comment'
    SERVICE_CHECK_UPDATE = 'service_check_update'
    SERVICE_MONITORINGS_SWITCH_STATUS = 'service_monitorings_switch_status'
    SBS_RUN_CHECK = 'sbs_run_check'
    SBS_RESULTS_SEE = 'sbs_results_see'
    SBS_PROFILE_UPDATE = 'sbs_profile_update'
    SBS_PROFILE_SEE = 'sbs_profile_see'
    TICKET_CREATE = 'ticket_create'
    TICKET_SEE = 'ticket_see'
    SUPPORT_PRIORITY_SWITCH = 'support_priority_switch'


DEFAULT_MASK = set()
ROLES = {
    "admin": DEFAULT_MASK | {
        PermissionKind.SERVICE_CREATE,
        PermissionKind.SERVICE_SEE,
        PermissionKind.SERVICE_STATUS_SWITCH,
        PermissionKind.LABEL_CREATE,
        PermissionKind.CHANGE_PARENT_LABEL,
        PermissionKind.PARENT_CONFIG_CREATE,
        PermissionKind.CONFIG_MARK_ACTIVE,
        PermissionKind.CONFIG_MARK_TEST,
        PermissionKind.CONFIG_MARK_EXPERIMENT,
        PermissionKind.CONFIG_CREATE,
        PermissionKind.CONFIG_ARCHIVE,
        PermissionKind.CONFIG_MODERATE,
        PermissionKind.HIDDEN_FIELDS_SEE,
        PermissionKind.HIDDEN_FIELDS_UPDATE,
        PermissionKind.TOKEN_UPDATE,
        PermissionKind.AUTH_GRANT,
        PermissionKind.SERVICE_COMMENT,
        PermissionKind.SERVICE_CHECK_UPDATE,
        PermissionKind.SERVICE_MONITORINGS_SWITCH_STATUS,
        PermissionKind.SBS_RUN_CHECK,
        PermissionKind.SBS_RESULTS_SEE,
        PermissionKind.SBS_PROFILE_UPDATE,
        PermissionKind.SBS_PROFILE_SEE,
        PermissionKind.TICKET_CREATE,
        PermissionKind.TICKET_SEE,
        PermissionKind.SUPPORT_PRIORITY_SWITCH
    },
    "guest": DEFAULT_MASK,
    "observer": DEFAULT_MASK | {
        PermissionKind.SERVICE_SEE,
    },
    "external_user": DEFAULT_MASK | {
        PermissionKind.SERVICE_SEE,
        PermissionKind.CONFIG_MARK_TEST,
        PermissionKind.CONFIG_MARK_ACTIVE,
        PermissionKind.CONFIG_CREATE,
        PermissionKind.CONFIG_ARCHIVE,
    },
    "internal_user": DEFAULT_MASK | {
        PermissionKind.SERVICE_SEE,
        PermissionKind.CONFIG_MARK_TEST,
        PermissionKind.CONFIG_MARK_ACTIVE,
        PermissionKind.CONFIG_CREATE,
        PermissionKind.CONFIG_ARCHIVE,
    },
}


DEFAULT_WEIGHT = 0
ROLES_WEIGHT = {k: DEFAULT_WEIGHT for k in ROLES.keys()}
ROLES_WEIGHT.update({
    "admin": 30,
    "internal_user": 20,
    "external_user": 20,
    "observer": 10,
})


DEFAULT_ADMIN_USERNAME = "ADMIN"

IDM_ROLES = {r: dict(set=r, name={"en": r, "ru": r}) for r in ROLES if r not in {"guest", "external_user"}}
IDM_ROLES["admin"]["name"]["ru"] = u"Администратор"
IDM_ROLES["internal_user"]["name"]["ru"] = u"Пользователь"
IDM_ROLES["observer"]["name"]["ru"] = u"Наблюдатель"
IDM_ROLES["admin"]["name"]["en"] = u"Administrator"
IDM_ROLES["internal_user"]["name"]["en"] = u"User"
IDM_ROLES["observer"]["name"]["en"] = u"Observer"
