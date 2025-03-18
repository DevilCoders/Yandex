CREATE TABLE public.permissions (
    uid BIGINT,
    role character varying(63) NOT NULL,
    node character varying(127),
    PRIMARY KEY(uid, node)
);

ALTER TABLE public.audit_log ALTER COLUMN action TYPE VARCHAR(63) USING action::text;
ALTER TYPE public.audit_action RENAME TO audit_action_old;
CREATE TYPE public.audit_action AS ENUM('config_active',
    'config_test',
    'service_create',
    'auth_grant');
ALTER TABLE public.audit_log ALTER COLUMN action TYPE public.audit_action USING action::public.audit_action;
DROP TYPE public.audit_action_old;

ALTER TABLE public.audit_log ALTER COLUMN service_id DROP NOT NULL;
