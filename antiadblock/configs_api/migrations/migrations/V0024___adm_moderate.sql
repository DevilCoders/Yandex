ALTER TABLE public.audit_log ALTER COLUMN action TYPE VARCHAR(63) USING action::text;
ALTER TYPE public.audit_action RENAME TO audit_action_old;
CREATE TYPE public.audit_action AS ENUM(
    'config_mark_active',
    'config_mark_test',
    'config_archive',
    'config_moderate',
    'service_create',
    'service_status_switch',
    'auth_grant'
);
ALTER TABLE public.audit_log ALTER COLUMN action TYPE public.audit_action USING action::public.audit_action;
DROP TYPE public.audit_action_old;

ALTER TABLE public.config_statuses ALTER COLUMN status TYPE VARCHAR(63) USING status::text;
ALTER TYPE public.config_status RENAME TO config_status_old;
CREATE TYPE public.config_status AS ENUM (
    'active',
    'test',
    'approved',
    'declined'
);
ALTER TABLE public.config_statuses ALTER COLUMN status TYPE public.config_status USING status::public.config_status;
DROP TYPE public.config_status_old;

ALTER TABLE public.config_statuses ADD COLUMN comment character varying(2048);

CREATE UNIQUE INDEX idx_config_statuses_config_id_status ON public.config_statuses USING BTREE (config_id, status);

INSERT INTO public.config_statuses(config_id, status, comment) SELECT config_id, 'approved', 'auto approve on migrate' FROM config_statuses WHERE status = 'active';