--
-- PostgreSQL database dump
--

-- Dumped from database version 11.5 (Debian 11.5-1.pgdg90+1)
-- Dumped by pg_dump version 11.5 (Ubuntu 11.5-1.pgdg18.04+1)

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

--
-- Name: public; Type: SCHEMA; Schema: -; Owner: -
--

CREATE SCHEMA public;


--
-- Name: SCHEMA public; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON SCHEMA public IS 'standard public schema';


--
-- Name: instance_type; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public.instance_type AS ENUM (
    'host',
    'svm'
);


--
-- Name: TYPE instance_type; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON TYPE public.instance_type IS 'Type to distinguish hosts and svms';


SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: host_configs_info; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.host_configs_info (
    id boolean DEFAULT true NOT NULL,
    version integer NOT NULL,
    CONSTRAINT host_configs_info_ck_onerow CHECK (id)
);


--
-- Name: TABLE host_configs_info; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON TABLE public.host_configs_info IS 'Info about host configs version (CLOUD-35467)';


--
-- Name: COLUMN host_configs_info.version; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON COLUMN public.host_configs_info.version IS 'Auto-incrementing version of host configs (CLOUD-35467)';


--
-- Name: CONSTRAINT host_configs_info_ck_onerow ON host_configs_info; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON CONSTRAINT host_configs_info_ck_onerow ON public.host_configs_info IS 'Ensure only one row';


--
-- Name: hw_props; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.hw_props (
    id integer NOT NULL,
    dynamic_config json
);


--
-- Name: TABLE hw_props; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON TABLE public.hw_props IS 'Hosts specific data table (CLOUD-27133)';


--
-- Name: COLUMN hw_props.dynamic_config; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON COLUMN public.hw_props.dynamic_config IS 'Replacement for host_config from cluster_configs repo (CLOUD-27133)';


--
-- Name: instance_group_release_sq_pkey; Type: SEQUENCE; Schema: public; Owner: -
--

CREATE SEQUENCE public.instance_group_release_sq_pkey
    START WITH 1
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    CACHE 1;


--
-- Name: instance_group_releases; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.instance_group_releases (
    id integer DEFAULT nextval('public.instance_group_release_sq_pkey'::regclass) NOT NULL,
    url text,
    image_id text,
    instance_group_id integer NOT NULL
);


--
-- Name: TABLE instance_group_releases; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON TABLE public.instance_group_releases IS 'Releses (images) for instance groups (currently 1:1 relation to <instance_groups> table) (CLOUD-33565)';


--
-- Name: instance_group_sq_pkey; Type: SEQUENCE; Schema: public; Owner: -
--

CREATE SEQUENCE public.instance_group_sq_pkey
    START WITH 1
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    CACHE 1;


--
-- Name: instance_groups; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.instance_groups (
    id integer DEFAULT nextval('public.instance_group_sq_pkey'::regclass) NOT NULL,
    name text NOT NULL,
    stand_id integer NOT NULL
);


--
-- Name: TABLE instance_groups; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON TABLE public.instance_groups IS 'Instance groups attached to stands (CLOUD-33565)';


--
-- Name: instance_salt_role_package_sq_pkey; Type: SEQUENCE; Schema: public; Owner: -
--

CREATE SEQUENCE public.instance_salt_role_package_sq_pkey
    START WITH 1
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    CACHE 1;


--
-- Name: instance_salt_role_packages; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.instance_salt_role_packages (
    id integer DEFAULT nextval('public.instance_salt_role_package_sq_pkey'::regclass) NOT NULL,
    instance_salt_role_id integer NOT NULL,
    package_name text NOT NULL,
    target_version text NOT NULL
);


--
-- Name: TABLE instance_salt_role_packages; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON TABLE public.instance_salt_role_packages IS 'Packages with version for instance and salt role';


--
-- Name: instance_salt_role_sq_pkey; Type: SEQUENCE; Schema: public; Owner: -
--

CREATE SEQUENCE public.instance_salt_role_sq_pkey
    START WITH 1
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    CACHE 1;


--
-- Name: instance_salt_roles; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.instance_salt_roles (
    id integer DEFAULT nextval('public.instance_salt_role_sq_pkey'::regclass) NOT NULL,
    instance_id integer NOT NULL,
    salt_role_id integer NOT NULL
);


--
-- Name: TABLE instance_salt_roles; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON TABLE public.instance_salt_roles IS 'Instance salt roles (CLOUD-36446)';


--
-- Name: instance_sq_pkey; Type: SEQUENCE; Schema: public; Owner: -
--

CREATE SEQUENCE public.instance_sq_pkey
    START WITH 1
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    CACHE 1;


--
-- Name: instances; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.instances (
    id integer DEFAULT nextval('public.instance_sq_pkey'::regclass) NOT NULL,
    fqdn text NOT NULL,
    type public.instance_type NOT NULL,
    stand_id integer
);


--
-- Name: TABLE instances; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON TABLE public.instances IS 'Table with all bootstrap hosts and svms';


--
-- Name: COLUMN instances.fqdn; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON COLUMN public.instances.fqdn IS 'Host fqdn';


--
-- Name: COLUMN instances.type; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON COLUMN public.instances.type IS 'Instance type';


--
-- Name: lock_sq_pkey; Type: SEQUENCE; Schema: public; Owner: -
--

CREATE SEQUENCE public.lock_sq_pkey
    START WITH 1
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    CACHE 1;


--
-- Name: locked_instances; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.locked_instances (
    instance_id integer NOT NULL,
    lock_id integer NOT NULL
);


--
-- Name: TABLE locked_instances; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON TABLE public.locked_instances IS 'Locked hosts ';


--
-- Name: locks; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.locks (
    id integer DEFAULT nextval('public.lock_sq_pkey'::regclass) NOT NULL,
    owner text NOT NULL,
    description text NOT NULL,
    hb_timeout integer NOT NULL,
    expired_at timestamp without time zone NOT NULL,
    CONSTRAINT locks_ck_hb_timeout_nonnegative CHECK ((hb_timeout >= 0))
);


--
-- Name: TABLE locks; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON TABLE public.locks IS 'List of currently acquired locks';


--
-- Name: COLUMN locks.owner; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON COLUMN public.locks.owner IS 'Lock owner (should be valid staff user)';


--
-- Name: COLUMN locks.description; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON COLUMN public.locks.description IS 'Extended lock description';


--
-- Name: COLUMN locks.hb_timeout; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON COLUMN public.locks.hb_timeout IS 'Heartbeat timeout in seconds (lock must be pinged every <hb_timeout> seconds)';


--
-- Name: COLUMN locks.expired_at; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON COLUMN public.locks.expired_at IS 'Lock expiration timestamp (in UTC)';


--
-- Name: salt_role_sq_pkey; Type: SEQUENCE; Schema: public; Owner: -
--

CREATE SEQUENCE public.salt_role_sq_pkey
    START WITH 1
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    CACHE 1;


--
-- Name: salt_roles; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.salt_roles (
    id integer DEFAULT nextval('public.salt_role_sq_pkey'::regclass) NOT NULL,
    name text NOT NULL
);


--
-- Name: TABLE salt_roles; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON TABLE public.salt_roles IS 'Salt roles for instances (CLOUD-36446)';


--
-- Name: scheme_info; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.scheme_info (
    id boolean DEFAULT true NOT NULL,
    version text NOT NULL,
    migrating_to_version text,
    CONSTRAINT scheme_info_ck_onerow CHECK (id)
);


--
-- Name: TABLE scheme_info; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON TABLE public.scheme_info IS 'Database scheme info (version, etc.). Can be used in migration process';


--
-- Name: COLUMN scheme_info.version; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON COLUMN public.scheme_info.version IS 'Scheme version';


--
-- Name: CONSTRAINT scheme_info_ck_onerow ON scheme_info; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON CONSTRAINT scheme_info_ck_onerow ON public.scheme_info IS 'Check for having only one line in table.';


--
-- Name: stand_cluster_map_sq_pkey; Type: SEQUENCE; Schema: public; Owner: -
--

CREATE SEQUENCE public.stand_cluster_map_sq_pkey
    START WITH 1
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    CACHE 1;


--
-- Name: stand_cluster_maps; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.stand_cluster_maps (
    id integer DEFAULT nextval('public.stand_cluster_map_sq_pkey'::regclass) NOT NULL,
    stand_id integer NOT NULL,
    grains jsonb,
    cluster_configs_version integer,
    yc_ci_version text,
    bootstrap_templates_version text
);


--
-- Name: TABLE stand_cluster_maps; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON TABLE public.stand_cluster_maps IS 'Stand cluster maps (1-to-1 relation with <stands>) (CLOUD-35467)';


--
-- Name: stand_sq_pkey; Type: SEQUENCE; Schema: public; Owner: -
--

CREATE SEQUENCE public.stand_sq_pkey
    START WITH 1
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    CACHE 1;


--
-- Name: stands; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.stands (
    id integer DEFAULT nextval('public.stand_sq_pkey'::regclass) NOT NULL,
    name text NOT NULL
);


--
-- Name: TABLE stands; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON TABLE public.stands IS 'All bootstrap stances (hw-labs/testing/pre-prod/prod)';


--
-- Name: COLUMN stands.name; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON COLUMN public.stands.name IS 'Stand name';


--
-- Name: svm_props; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.svm_props (
    id integer NOT NULL,
    dynamic_config json
);


--
-- Name: TABLE svm_props; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON TABLE public.svm_props IS 'Svms specific data table (CLOUD-27133)';


--
-- Name: COLUMN svm_props.dynamic_config; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON COLUMN public.svm_props.dynamic_config IS 'Replacement for host_config from cluster_configs repo (CLOUD-27133)';


--
-- Name: host_configs_info host_configs_info_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.host_configs_info
    ADD CONSTRAINT host_configs_info_pkey PRIMARY KEY (id);


--
-- Name: hw_props hw_props_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.hw_props
    ADD CONSTRAINT hw_props_pkey PRIMARY KEY (id);


--
-- Name: instance_group_releases instance_group_release_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.instance_group_releases
    ADD CONSTRAINT instance_group_release_pkey PRIMARY KEY (id);


--
-- Name: instance_group_releases instance_group_release_uq_instance_group_id; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.instance_group_releases
    ADD CONSTRAINT instance_group_release_uq_instance_group_id UNIQUE (instance_group_id);


--
-- Name: CONSTRAINT instance_group_release_uq_instance_group_id ON instance_group_releases; Type: COMMENT; Schema: public; Owner: -
--

COMMENT ON CONSTRAINT instance_group_release_uq_instance_group_id ON public.instance_group_releases IS '1:1 relation to <instance_groups> table';


--
-- Name: instance_groups instance_groups_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.instance_groups
    ADD CONSTRAINT instance_groups_pkey PRIMARY KEY (id);


--
-- Name: instance_groups instance_groups_uq_name_stand_id; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.instance_groups
    ADD CONSTRAINT instance_groups_uq_name_stand_id UNIQUE (name, stand_id);


--
-- Name: instance_salt_role_packages instance_salt_role_packages_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.instance_salt_role_packages
    ADD CONSTRAINT instance_salt_role_packages_pkey PRIMARY KEY (id);


--
-- Name: instance_salt_roles instance_salt_roles_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.instance_salt_roles
    ADD CONSTRAINT instance_salt_roles_pkey PRIMARY KEY (id);


--
-- Name: instance_salt_roles instance_salt_roles_uq_instance_id_salt_role_id; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.instance_salt_roles
    ADD CONSTRAINT instance_salt_roles_uq_instance_id_salt_role_id UNIQUE (instance_id, salt_role_id);


--
-- Name: instance_salt_role_packages instance_salt_roles_uq_instance_salt_role_id_package_name; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.instance_salt_role_packages
    ADD CONSTRAINT instance_salt_roles_uq_instance_salt_role_id_package_name UNIQUE (instance_salt_role_id, package_name);


--
-- Name: instances instances_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.instances
    ADD CONSTRAINT instances_pkey PRIMARY KEY (id);


--
-- Name: instances instances_uq_fqdn; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.instances
    ADD CONSTRAINT instances_uq_fqdn UNIQUE (fqdn);


--
-- Name: locked_instances locked_instances_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.locked_instances
    ADD CONSTRAINT locked_instances_pkey PRIMARY KEY (instance_id, lock_id);


--
-- Name: locked_instances locked_instances_uq_instnace_id; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.locked_instances
    ADD CONSTRAINT locked_instances_uq_instnace_id UNIQUE (instance_id);


--
-- Name: locks locks_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.locks
    ADD CONSTRAINT locks_pkey PRIMARY KEY (id);


--
-- Name: salt_roles salt_roles_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.salt_roles
    ADD CONSTRAINT salt_roles_pkey PRIMARY KEY (id);


--
-- Name: salt_roles salt_roles_uq_name; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.salt_roles
    ADD CONSTRAINT salt_roles_uq_name UNIQUE (name);


--
-- Name: scheme_info scheme_info_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.scheme_info
    ADD CONSTRAINT scheme_info_pkey PRIMARY KEY (id);


--
-- Name: stand_cluster_maps stand_cluster_map_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.stand_cluster_maps
    ADD CONSTRAINT stand_cluster_map_pkey PRIMARY KEY (id);


--
-- Name: stand_cluster_maps stand_cluster_map_uq_stand_id; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.stand_cluster_maps
    ADD CONSTRAINT stand_cluster_map_uq_stand_id UNIQUE (stand_id);


--
-- Name: stands stands_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.stands
    ADD CONSTRAINT stands_pkey PRIMARY KEY (id);


--
-- Name: stands stands_uq_name; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.stands
    ADD CONSTRAINT stands_uq_name UNIQUE (name);


--
-- Name: svm_props svm_props_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.svm_props
    ADD CONSTRAINT svm_props_pkey PRIMARY KEY (id);


--
-- Name: hw_props hw_props_instances_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.hw_props
    ADD CONSTRAINT hw_props_instances_fkey FOREIGN KEY (id) REFERENCES public.instances(id) MATCH FULL ON DELETE CASCADE;


--
-- Name: instance_group_releases instance_group_releases_instance_groups_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.instance_group_releases
    ADD CONSTRAINT instance_group_releases_instance_groups_fkey FOREIGN KEY (instance_group_id) REFERENCES public.instance_groups(id) MATCH FULL ON DELETE CASCADE;


--
-- Name: instance_groups instance_groups_stands_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.instance_groups
    ADD CONSTRAINT instance_groups_stands_fkey FOREIGN KEY (stand_id) REFERENCES public.stands(id) MATCH FULL ON DELETE CASCADE;


--
-- Name: instance_salt_role_packages instance_salt_role_packages_instance_salt_roles_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.instance_salt_role_packages
    ADD CONSTRAINT instance_salt_role_packages_instance_salt_roles_fkey FOREIGN KEY (instance_salt_role_id) REFERENCES public.instance_salt_roles(id) MATCH FULL ON DELETE CASCADE;


--
-- Name: instance_salt_roles instance_salt_roles_instances_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.instance_salt_roles
    ADD CONSTRAINT instance_salt_roles_instances_fkey FOREIGN KEY (instance_id) REFERENCES public.instances(id) MATCH FULL ON DELETE CASCADE;


--
-- Name: instance_salt_roles instance_salt_roles_salt_roles_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.instance_salt_roles
    ADD CONSTRAINT instance_salt_roles_salt_roles_fkey FOREIGN KEY (salt_role_id) REFERENCES public.salt_roles(id) MATCH FULL ON DELETE CASCADE;


--
-- Name: instances instances_stands_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.instances
    ADD CONSTRAINT instances_stands_fkey FOREIGN KEY (stand_id) REFERENCES public.stands(id) MATCH FULL ON DELETE SET NULL;


--
-- Name: locked_instances locked_instances_instances_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.locked_instances
    ADD CONSTRAINT locked_instances_instances_fkey FOREIGN KEY (instance_id) REFERENCES public.instances(id) MATCH FULL ON DELETE CASCADE;


--
-- Name: locked_instances locked_instances_locks_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.locked_instances
    ADD CONSTRAINT locked_instances_locks_fkey FOREIGN KEY (lock_id) REFERENCES public.locks(id) MATCH FULL ON DELETE CASCADE;


--
-- Name: stand_cluster_maps stand_cluster_maps_cluster_maps_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.stand_cluster_maps
    ADD CONSTRAINT stand_cluster_maps_cluster_maps_fkey FOREIGN KEY (stand_id) REFERENCES public.stands(id) MATCH FULL ON DELETE CASCADE;


--
-- Name: svm_props svm_props_instances_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.svm_props
    ADD CONSTRAINT svm_props_instances_fkey FOREIGN KEY (id) REFERENCES public.instances(id) MATCH FULL ON DELETE CASCADE;


--
-- PostgreSQL database dump complete
--

