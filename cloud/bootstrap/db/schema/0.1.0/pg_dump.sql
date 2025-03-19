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
-- Name: instance_type; Type: TYPE; Schema: public; Owner: postgres
--

CREATE TYPE public.instance_type AS ENUM (
    'host',
    'svm'
);


--
-- Name: TYPE instance_type; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TYPE public.instance_type IS 'Type to distinguish hosts and svms';


SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: hw_props; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.hw_props (
    id integer NOT NULL,
    dynamic_config json
);


--
-- Name: TABLE hw_props; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.hw_props IS 'Hosts specific data table (CLOUD-27133)';


--
-- Name: COLUMN hw_props.dynamic_config; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.hw_props.dynamic_config IS 'Replacement for host_config from cluster_configs repo (CLOUD-27133)';


--
-- Name: instance_sq_pkey; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.instance_sq_pkey
    START WITH 1
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    CACHE 1;


--
-- Name: instances; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.instances (
    id integer DEFAULT nextval('public.instance_sq_pkey'::regclass) NOT NULL,
    fqdn text NOT NULL,
    type public.instance_type NOT NULL
);


--
-- Name: TABLE instances; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.instances IS 'Table with all bootstrap hosts and svms';


--
-- Name: COLUMN instances.fqdn; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.instances.fqdn IS 'Host fqdn';


--
-- Name: COLUMN instances.type; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.instances.type IS 'Instance type';


--
-- Name: lock_sq_pkey; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.lock_sq_pkey
    START WITH 1
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    CACHE 1;


--
-- Name: locked_instances; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.locked_instances (
    instance_id integer NOT NULL,
    lock_id integer NOT NULL
);


--
-- Name: TABLE locked_instances; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.locked_instances IS 'Locked hosts ';


--
-- Name: locks; Type: TABLE; Schema: public; Owner: postgres
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
-- Name: TABLE locks; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.locks IS 'List of currently acquired locks';


--
-- Name: COLUMN locks.owner; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.locks.owner IS 'Lock owner (should be valid staff user)';


--
-- Name: COLUMN locks.description; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.locks.description IS 'Extended lock description';


--
-- Name: COLUMN locks.hb_timeout; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.locks.hb_timeout IS 'Heartbeat timeout in seconds (lock must be pinged every <hb_timeout> seconds)';


--
-- Name: COLUMN locks.expired_at; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.locks.expired_at IS 'Lock expiration timestamp (in UTC)';


--
-- Name: scheme_info; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.scheme_info (
    id boolean DEFAULT true NOT NULL,
    version text NOT NULL,
    migrating_to_version text,
    CONSTRAINT scheme_info_ck_onerow CHECK (id)
);


--
-- Name: TABLE scheme_info; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.scheme_info IS 'Database scheme info (version, etc.). Can be used in migration process';


--
-- Name: COLUMN scheme_info.version; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.scheme_info.version IS 'Scheme version';


--
-- Name: CONSTRAINT scheme_info_ck_onerow ON scheme_info; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON CONSTRAINT scheme_info_ck_onerow ON public.scheme_info IS 'Check for having only one line in table.';


--
-- Name: svm_props; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.svm_props (
    id integer NOT NULL,
    dynamic_config json
);


--
-- Name: TABLE svm_props; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.svm_props IS 'Svms specific data table (CLOUD-27133)';


--
-- Name: COLUMN svm_props.dynamic_config; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.svm_props.dynamic_config IS 'Replacement for host_config from cluster_configs repo (CLOUD-27133)';


--
-- Name: hw_props hw_props_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.hw_props
    ADD CONSTRAINT hw_props_pkey PRIMARY KEY (id);


--
-- Name: instances instances_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.instances
    ADD CONSTRAINT instances_pkey PRIMARY KEY (id);


--
-- Name: instances instances_uq_fqdn; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.instances
    ADD CONSTRAINT instances_uq_fqdn UNIQUE (fqdn);


--
-- Name: locked_instances locked_instances_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.locked_instances
    ADD CONSTRAINT locked_instances_pkey PRIMARY KEY (instance_id, lock_id);


--
-- Name: locked_instances locked_instances_uq_instnace_id; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.locked_instances
    ADD CONSTRAINT locked_instances_uq_instnace_id UNIQUE (instance_id);


--
-- Name: locks locks_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.locks
    ADD CONSTRAINT locks_pkey PRIMARY KEY (id);


--
-- Name: scheme_info scheme_info_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.scheme_info
    ADD CONSTRAINT scheme_info_pkey PRIMARY KEY (id);


--
-- Name: svm_props svm_props_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.svm_props
    ADD CONSTRAINT svm_props_pkey PRIMARY KEY (id);


--
-- Name: hw_props hw_props_instances_fkey; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.hw_props
    ADD CONSTRAINT hw_props_instances_fkey FOREIGN KEY (id) REFERENCES public.instances(id) MATCH FULL ON DELETE CASCADE;


--
-- Name: locked_instances locked_instances_instances_fkey; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.locked_instances
    ADD CONSTRAINT locked_instances_instances_fkey FOREIGN KEY (instance_id) REFERENCES public.instances(id) MATCH FULL ON DELETE CASCADE;


--
-- Name: locked_instances locked_instances_locks_fkey; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.locked_instances
    ADD CONSTRAINT locked_instances_locks_fkey FOREIGN KEY (lock_id) REFERENCES public.locks(id) MATCH FULL ON DELETE CASCADE;


--
-- Name: svm_props svm_props_instances_fkey; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.svm_props
    ADD CONSTRAINT svm_props_instances_fkey FOREIGN KEY (id) REFERENCES public.instances(id) MATCH FULL ON DELETE CASCADE;


--
-- PostgreSQL database dump complete
--

