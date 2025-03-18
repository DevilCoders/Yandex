export const ACTIONS = {
    CONFIG_TEST: 'config_mark_test',
    CONFIG_ACTIVE: 'config_mark_active',
    SERVICE_CREATE: 'service_create',
    SERVICE_STATUS_SWITCH: 'service_status_switch',
    SERVICE_MONITORINGS_SWITCH: 'service_monitorings_switch_status',
    SERVICE_UPDATE_SBS_PROFILE: 'sbs_profile_update',
    CONFIG_ARCHIVE: 'config_archive',
    CONFIG_MODERATE: 'config_moderate',
    CONFIG_APPROVED: 'config_approve',
    CONFIG_DECLINED: 'config_decline',
    TICKET_CREATE: 'ticket_create',
    TICKET_SEE: 'ticket_see',
    SUPPORT_PRIORITY_SWITCH: 'support_priority_switch'
};

export const ACTIONS_COLORS = {
    [ACTIONS.CONFIG_TEST]: 'blue',
    [ACTIONS.CONFIG_ACTIVE]: 'green',
    [ACTIONS.CONFIG_APPROVED]: 'purple',
    [ACTIONS.CONFIG_DECLINED]: 'red'
};
