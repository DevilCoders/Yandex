from antiadblock.configs_api.lib.auth.permission_models import PermissionKind
from antiadblock.configs_api.lib.utils import ListableEnum


class ServiceStatus(ListableEnum):
    OK = 'ok'
    INACTIVE = 'inactive'


class ServiceSupportPriority(ListableEnum):
    CRITICAL = 'critical'
    MAJOR = 'major'
    MINOR = 'minor'
    OTHER = 'other'


class ConfigStatus(ListableEnum):
    ACTIVE = 'active'
    TEST = 'test'
    APPROVED = 'approved'
    DECLINED = 'declined'


class AuditAction(ListableEnum):
    CONFIG_TEST = PermissionKind.CONFIG_MARK_TEST
    CONFIG_ACTIVE = PermissionKind.CONFIG_MARK_ACTIVE
    CONFIG_EXPERIMENT = PermissionKind.CONFIG_MARK_EXPERIMENT
    TICKET_CREATE = PermissionKind.TICKET_CREATE
    CONFIG_ARCHIVE = PermissionKind.CONFIG_ARCHIVE
    CONFIG_MODERATE = PermissionKind.CONFIG_MODERATE
    PARENT_CONFIG_CREATE = PermissionKind.PARENT_CONFIG_CREATE
    SERVICE_CREATE = PermissionKind.SERVICE_CREATE
    SERVICE_STATUS_SWITCH = PermissionKind.SERVICE_STATUS_SWITCH
    CHANGE_PARENT_LABEL = PermissionKind.CHANGE_PARENT_LABEL
    LABEL_CREATE = PermissionKind.LABEL_CREATE
    AUTH_GRANT = PermissionKind.AUTH_GRANT
    SERVICE_MONITORINGS_SWITCH_STATUS = PermissionKind.SERVICE_MONITORINGS_SWITCH_STATUS
    ARGUS_PROFILE_UPDATE = PermissionKind.SBS_PROFILE_UPDATE
    SUPPORT_PRIORITY_SWITCH = PermissionKind.SUPPORT_PRIORITY_SWITCH


class CheckState(ListableEnum):
    GREEN = 'green'
    YELLOW = 'yellow'
    RED = 'red'


class SbSCheckStatus(ListableEnum):
    NEW = 'new'
    IN_PROGRESS = 'in_progress'
    FAIL = 'fail'
    SUCCESS = 'success'


class TrendType(ListableEnum):
    NEGATIVE_TREND = 'negative_trend'
    MONEY_DROP = 'money_drop'


class ArgusLogStatus(ListableEnum):
    NEW = 'new'
    WAIT_CONFIRMATION = 'wait_confirmation'
    SUCCESS = 'success'


class TrendDeviceType(ListableEnum):
    DESKTOP = 'desktop'
    MOBILE = 'mobile'
