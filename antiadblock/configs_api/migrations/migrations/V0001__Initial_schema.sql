--
-- PostgreSQL database dump
--

-- Dumped from database version 9.6.8
-- Dumped by pg_dump version 9.6.8

SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET row_security = off;
SET default_tablespace = '';
SET default_with_oids = false;


CREATE TYPE public.audit_action AS ENUM (
    'config_active',
    'config_test',
    'service_create'
);


CREATE TYPE public.config_status AS ENUM (
    'active',
    'inactive',
    'test'
);

CREATE TYPE public.service_status AS ENUM (
    'error',
    'inactive',
    'ok',
    'wip'
);

-- ALTER TYPE public.service_status OWNER TO antiadb;

--
-- Name: audit_log; Type: TABLE; Schema: public; Owner: antiadb
--
CREATE TABLE public.audit_log (
    id BIGSERIAL PRIMARY KEY ,
    user_id bigint,
    date timestamp without time zone NOT NULL,
    service_id character varying(127) NOT NULL,
    action public.audit_action NOT NULL,
    params jsonb NOT NULL
);

-- ALTER TABLE public.audit_log_id_seq OWNER TO antiadb;

--
-- Name: configs; Type: TABLE; Schema: public; Owner: antiadb
--
CREATE TABLE public.configs (
    id BIGSERIAL PRIMARY KEY,
    comment character varying(60) NOT NULL,
    data jsonb NOT NULL,
    service_id character varying(127) NOT NULL,
    created timestamp without time zone NOT NULL,
    status public.config_status NOT NULL,
    creator_id bigint,
    parent_id integer
);

--
-- Name: services; Type: TABLE; Schema: public; Owner: antiadb
--
CREATE TABLE public.services (
    id character varying(127) NOT NULL PRIMARY KEY,
    name character varying(60) NOT NULL,
    status public.service_status NOT NULL,
    domain character varying(127) NOT NULL UNIQUE,
    owner_id bigint
);

-- ALTER TABLE public.services OWNER TO antiadb;
CREATE INDEX idx_audit_entry_service_id_date ON public.audit_log USING btree (service_id, date DESC);
CREATE INDEX idx_config_service_id_created ON public.configs USING btree (service_id, created);
CREATE INDEX idx_service_owner_id ON public.services USING btree (owner_id);
ALTER TABLE ONLY public.audit_log
    ADD CONSTRAINT audit_log_service_id_fkey FOREIGN KEY (service_id) REFERENCES public.services(id);
ALTER TABLE ONLY public.configs
    ADD CONSTRAINT configs_parent_id_fkey FOREIGN KEY (parent_id) REFERENCES public.configs(id);
ALTER TABLE ONLY public.configs
    ADD CONSTRAINT configs_service_id_fkey FOREIGN KEY (service_id) REFERENCES public.services(id);
