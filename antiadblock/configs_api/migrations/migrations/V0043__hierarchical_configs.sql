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
    'parent_config_create'
);
ALTER TABLE public.audit_log ALTER COLUMN action TYPE public.audit_action USING action::public.audit_action;
DROP TYPE public.audit_action_old;

CREATE TYPE public.device_type AS ENUM (
    'desktop',
    'mobile'
);

ALTER TABLE public.configs ADD COLUMN parent_label_id character varying(127);
ALTER TABLE public.configs ADD COLUMN device_type public.device_type;
ALTER TABLE public.configs ADD COLUMN exp_id character varying(127);
ALTER TABLE public.configs ADD COLUMN label_id character varying(127);
ALTER TABLE public.configs ALTER COLUMN service_id DROP NOT NULL;

ALTER TABLE public.audit_log ALTER COLUMN service_id DROP NOT NULL;

UPDATE configs
SET label_id=subquery.service_id,
    parent_label_id = 'ROOT'
FROM (SELECT id, service_id FROM configs) as subquery
WHERE configs.id = subquery.id;

INSERT INTO configs (comment, data, created, label_id, parent_label_id) VALUES ('Create ROOT on migration', '{}'::jsonb, LOCALTIMESTAMP, 'ROOT', NULL);
INSERT INTO audit_log (date, action, params) VALUES (LOCALTIMESTAMP, 'parent_config_create', '{"config_comment": "Create ROOT on migration"}'::jsonb);
INSERT INTO config_statuses VALUES (currval('configs_id_seq'), 'active'), (currval('configs_id_seq'), 'test'), (currval('configs_id_seq'), 'approved');

ALTER TABLE public.configs ALTER COLUMN label_id SET NOT NULL;
ALTER TABLE public.configs ALTER COLUMN parent_label_id SET DEFAULT 'ROOT';
