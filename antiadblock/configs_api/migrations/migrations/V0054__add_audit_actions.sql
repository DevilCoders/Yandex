ALTER TABLE public.audit_log ALTER COLUMN action TYPE VARCHAR(63) USING action::text;
ALTER TYPE public.audit_action RENAME TO audit_action_old;
CREATE TYPE public.audit_action AS ENUM(
    'config_mark_active',
    'config_mark_test',
    'config_archive',
    'config_moderate',
    'service_create',
    'service_status_switch',
    'auth_grant',
    'service_monitorings_switch_status',
    'sbs_profile_update',
    'parent_config_create',
    'change_parent_label',
    'label_create',
    'config_select_device_type',
    'config_mark_experiment',
    'ticket_create',
    'support_priority_switch'
);
ALTER TABLE public.audit_log ALTER COLUMN action TYPE public.audit_action USING action::public.audit_action;
DROP TYPE public.audit_action_old;
