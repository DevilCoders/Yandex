CREATE TYPE public.sbs_check_state AS ENUM (
    'new',
    'in_progress',
    'fail',
    'success'
);

CREATE TABLE public.sbs_profiles (
    id BIGSERIAL PRIMARY KEY,
    date timestamp without time zone NOT NULL,
    service_id character varying(127) NOT NULL,
    urls_list jsonb NOT NULL DEFAULT '[]'::jsonb
);

ALTER TABLE ONLY public.sbs_profiles
    ADD CONSTRAINT sbs_profiles_service_id_fkey FOREIGN KEY (service_id) REFERENCES public.services(id);


CREATE TABLE public.sbs_runs (
    id BIGSERIAL PRIMARY KEY,
    status public.sbs_check_state NOT NULL,
    owner BIGINT NOT NULL,
    date timestamp without time zone NOT NULL,
    config_id integer NOT NULL,
    sandbox_id BIGINT NOT NULL,
    profile_id BIGINT NOT NULL
);

ALTER TABLE ONLY public.sbs_runs
    ADD CONSTRAINT sbs_runs_owner_fkey FOREIGN KEY (owner) REFERENCES public.user_logins(uid);

ALTER TABLE ONLY public.sbs_runs
    ADD CONSTRAINT sbs_runs_config_id_fkey FOREIGN KEY (config_id) REFERENCES public.configs(id);

ALTER TABLE ONLY public.sbs_runs
    ADD CONSTRAINT sbs_runs_profile_id_fkey FOREIGN KEY (profile_id) REFERENCES public.sbs_profiles(id);


CREATE TABLE public.sbs_results (
    id BIGSERIAL PRIMARY KEY,
    start_time timestamp without time zone NOT NULL,
    end_time timestamp without time zone NOT NULL,
    cases jsonb NOT NULL DEFAULT '[]'::jsonb,
    filters_lists jsonb NOT NULL DEFAULT '[]'::jsonb
);

ALTER TABLE ONLY public.sbs_results
    ADD CONSTRAINT sbs_results_owner_fkey FOREIGN KEY (id) REFERENCES public.sbs_runs(id);

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
    'sbs_profile_update');
ALTER TABLE public.audit_log ALTER COLUMN action TYPE public.audit_action USING action::public.audit_action;
DROP TYPE public.audit_action_old;