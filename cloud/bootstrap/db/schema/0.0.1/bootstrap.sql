-- Database generated with pgModeler (PostgreSQL Database Modeler).
-- pgModeler  version: 0.9.1-beta
-- PostgreSQL version: 10.0
-- Project Site: pgmodeler.com.br
-- Model Author: ---


-- Database creation must be done outside an multicommand file.
-- These commands were put in this file only for convenience.
-- -- object: new_database | type: DATABASE --
-- -- DROP DATABASE IF EXISTS new_database;
-- CREATE DATABASE new_database
-- ;
-- -- ddl-end --
-- 

-- object: public.scheme_info | type: TABLE --
-- DROP TABLE IF EXISTS public.scheme_info CASCADE;
CREATE TABLE public.scheme_info(
	id bool NOT NULL DEFAULT True,
	version text NOT NULL,
	migrating_to_version text,
	CONSTRAINT scheme_info_ck_onerow CHECK (id),
	CONSTRAINT scheme_info_pkey PRIMARY KEY (id)

);
-- ddl-end --
COMMENT ON TABLE public.scheme_info IS 'Database scheme info (version, etc.). Can be used in migration process';
-- ddl-end --
COMMENT ON COLUMN public.scheme_info.version IS 'Scheme version';
-- ddl-end --
COMMENT ON CONSTRAINT scheme_info_ck_onerow ON public.scheme_info  IS 'Check for having only one line in table.';
-- ddl-end --

-- object: public.host_sq_pkey | type: SEQUENCE --
-- DROP SEQUENCE IF EXISTS public.host_sq_pkey CASCADE;
CREATE SEQUENCE public.host_sq_pkey
	INCREMENT BY 1
	MINVALUE 0
	MAXVALUE 2147483647
	START WITH 1
	CACHE 1
	NO CYCLE
	OWNED BY NONE;
-- ddl-end --

-- object: public.hosts | type: TABLE --
-- DROP TABLE IF EXISTS public.hosts CASCADE;
CREATE TABLE public.hosts(
	id integer NOT NULL DEFAULT nextval('public.host_sq_pkey'::regclass),
	name text NOT NULL,
	CONSTRAINT hosts_pkey PRIMARY KEY (id)

);
-- ddl-end --
COMMENT ON TABLE public.hosts IS 'Table with all bootstrap hosts and svms';
-- ddl-end --
COMMENT ON COLUMN public.hosts.name IS 'Host fqdn';
-- ddl-end --

-- object: public.lock_sq_pkey | type: SEQUENCE --
-- DROP SEQUENCE IF EXISTS public.lock_sq_pkey CASCADE;
CREATE SEQUENCE public.lock_sq_pkey
	INCREMENT BY 1
	MINVALUE 0
	MAXVALUE 2147483647
	START WITH 1
	CACHE 1
	NO CYCLE
	OWNED BY NONE;
-- ddl-end --

-- object: public.locks | type: TABLE --
-- DROP TABLE IF EXISTS public.locks CASCADE;
CREATE TABLE public.locks(
	id integer NOT NULL DEFAULT nextval('public.lock_sq_pkey'::regclass),
	owner text NOT NULL,
	description text NOT NULL,
	hb_timeout integer NOT NULL,
	expired_at timestamp NOT NULL,
	CONSTRAINT locks_pkey PRIMARY KEY (id),
	CONSTRAINT locks_ck_hb_timeout_nonnegative CHECK (hb_timeout >= 0)

);
-- ddl-end --
COMMENT ON TABLE public.locks IS 'List of currently acquired locks';
-- ddl-end --
COMMENT ON COLUMN public.locks.owner IS 'Lock owner (should be valid staff user)';
-- ddl-end --
COMMENT ON COLUMN public.locks.description IS 'Extended lock description';
-- ddl-end --
COMMENT ON COLUMN public.locks.hb_timeout IS 'Heartbeat timeout in seconds (lock must be pinged every <hb_timeout> seconds)';
-- ddl-end --
COMMENT ON COLUMN public.locks.expired_at IS 'Lock expiration timestamp (in UTC)';
-- ddl-end --

-- object: public.locked_hosts | type: TABLE --
-- DROP TABLE IF EXISTS public.locked_hosts CASCADE;
CREATE TABLE public.locked_hosts(
	host_id integer NOT NULL,
	lock_id integer NOT NULL,
	CONSTRAINT locked_hosts_uq_host_id UNIQUE (host_id),
	CONSTRAINT locked_hosts_pkey PRIMARY KEY (host_id,lock_id)

);
-- ddl-end --
COMMENT ON TABLE public.locked_hosts IS 'Locked hosts ';
-- ddl-end --

-- object: locked_hosts_locks_fkey | type: CONSTRAINT --
-- ALTER TABLE public.locked_hosts DROP CONSTRAINT IF EXISTS locked_hosts_locks_fkey CASCADE;
ALTER TABLE public.locked_hosts ADD CONSTRAINT locked_hosts_locks_fkey FOREIGN KEY (lock_id)
REFERENCES public.locks (id) MATCH FULL
ON DELETE CASCADE ON UPDATE NO ACTION;
-- ddl-end --

-- object: locked_hosts_hosts_fkey | type: CONSTRAINT --
-- ALTER TABLE public.locked_hosts DROP CONSTRAINT IF EXISTS locked_hosts_hosts_fkey CASCADE;
ALTER TABLE public.locked_hosts ADD CONSTRAINT locked_hosts_hosts_fkey FOREIGN KEY (host_id)
REFERENCES public.hosts (id) MATCH FULL
ON DELETE CASCADE ON UPDATE NO ACTION;
-- ddl-end --


