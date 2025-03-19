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
-- Name: host_sq_pkey; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.host_sq_pkey
    START WITH 1
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    CACHE 1;


SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: hosts; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.hosts (
    id integer DEFAULT nextval('public.host_sq_pkey'::regclass) NOT NULL,
    name text NOT NULL
);


--
-- Name: TABLE hosts; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.hosts IS 'Table with all bootstrap hosts and svms';


--
-- Name: COLUMN hosts.name; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.hosts.name IS 'Host fqdn';


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
-- Name: locked_hosts; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.locked_hosts (
    host_id integer NOT NULL,
    lock_id integer NOT NULL
);


--
-- Name: TABLE locked_hosts; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.locked_hosts IS 'Locked hosts ';


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
-- Name: hosts hosts_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.hosts
    ADD CONSTRAINT hosts_pkey PRIMARY KEY (id);


--
-- Name: locked_hosts locked_hosts_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.locked_hosts
    ADD CONSTRAINT locked_hosts_pkey PRIMARY KEY (host_id, lock_id);


--
-- Name: locked_hosts locked_hosts_uq_host_id; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.locked_hosts
    ADD CONSTRAINT locked_hosts_uq_host_id UNIQUE (host_id);


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
-- Name: locked_hosts locked_hosts_hosts_fkey; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.locked_hosts
    ADD CONSTRAINT locked_hosts_hosts_fkey FOREIGN KEY (host_id) REFERENCES public.hosts(id) MATCH FULL ON DELETE CASCADE;


--
-- Name: locked_hosts locked_hosts_locks_fkey; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.locked_hosts
    ADD CONSTRAINT locked_hosts_locks_fkey FOREIGN KEY (lock_id) REFERENCES public.locks(id) MATCH FULL ON DELETE CASCADE;


--
-- PostgreSQL database dump complete
--

