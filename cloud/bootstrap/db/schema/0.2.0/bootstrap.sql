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

-- object: public.instance_type | type: TYPE --
-- DROP TYPE IF EXISTS public.instance_type CASCADE;
CREATE TYPE public.instance_type AS
 ENUM ('host','svm');
-- ddl-end --
COMMENT ON TYPE public.instance_type IS 'Type to distinguish hosts and svms';
-- ddl-end --

-- object: public.instance_sq_pkey | type: SEQUENCE --
-- DROP SEQUENCE IF EXISTS public.instance_sq_pkey CASCADE;
CREATE SEQUENCE public.instance_sq_pkey
	INCREMENT BY 1
	MINVALUE 0
	MAXVALUE 2147483647
	START WITH 1
	CACHE 1
	NO CYCLE
	OWNED BY NONE;
-- ddl-end --

-- object: public.instances | type: TABLE --
-- DROP TABLE IF EXISTS public.instances CASCADE;
CREATE TABLE public.instances(
	id integer NOT NULL DEFAULT nextval('public.instance_sq_pkey'::regclass),
	fqdn text NOT NULL,
	type public.instance_type NOT NULL,
	stand_id integer,
	CONSTRAINT instances_pkey PRIMARY KEY (id),
	CONSTRAINT instances_uq_fqdn UNIQUE (fqdn)

);
-- ddl-end --
COMMENT ON TABLE public.instances IS 'Table with all bootstrap hosts and svms';
-- ddl-end --
COMMENT ON COLUMN public.instances.fqdn IS 'Host fqdn';
-- ddl-end --
COMMENT ON COLUMN public.instances.type IS 'Instance type';
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

-- object: public.locked_instances | type: TABLE --
-- DROP TABLE IF EXISTS public.locked_instances CASCADE;
CREATE TABLE public.locked_instances(
	instance_id integer NOT NULL,
	lock_id integer NOT NULL,
	CONSTRAINT locked_instances_uq_instnace_id UNIQUE (instance_id),
	CONSTRAINT locked_instances_pkey PRIMARY KEY (instance_id,lock_id)

);
-- ddl-end --
COMMENT ON TABLE public.locked_instances IS 'Locked hosts ';
-- ddl-end --

-- object: public.hw_props | type: TABLE --
-- DROP TABLE IF EXISTS public.hw_props CASCADE;
CREATE TABLE public.hw_props(
	id integer NOT NULL,
	dynamic_config json,
	CONSTRAINT hw_props_pkey PRIMARY KEY (id)

);
-- ddl-end --
COMMENT ON TABLE public.hw_props IS 'Hosts specific data table (CLOUD-27133)';
-- ddl-end --
COMMENT ON COLUMN public.hw_props.dynamic_config IS 'Replacement for host_config from cluster_configs repo (CLOUD-27133)';
-- ddl-end --

-- object: public.svm_props | type: TABLE --
-- DROP TABLE IF EXISTS public.svm_props CASCADE;
CREATE TABLE public.svm_props(
	id integer NOT NULL,
	dynamic_config json,
	CONSTRAINT svm_props_pkey PRIMARY KEY (id)

);
-- ddl-end --
COMMENT ON TABLE public.svm_props IS 'Svms specific data table (CLOUD-27133)';
-- ddl-end --
COMMENT ON COLUMN public.svm_props.dynamic_config IS 'Replacement for host_config from cluster_configs repo (CLOUD-27133)';
-- ddl-end --

-- object: public.stand_sq_pkey | type: SEQUENCE --
-- DROP SEQUENCE IF EXISTS public.stand_sq_pkey CASCADE;
CREATE SEQUENCE public.stand_sq_pkey
	INCREMENT BY 1
	MINVALUE 0
	MAXVALUE 2147483647
	START WITH 1
	CACHE 1
	NO CYCLE
	OWNED BY NONE;
-- ddl-end --

-- object: public.stands | type: TABLE --
-- DROP TABLE IF EXISTS public.stands CASCADE;
CREATE TABLE public.stands(
	id integer NOT NULL DEFAULT nextval('public.stand_sq_pkey'::regclass),
	name text NOT NULL,
	cluster_map json,
	CONSTRAINT stands_pkey PRIMARY KEY (id),
	CONSTRAINT stands_uq_name UNIQUE (name)

);
-- ddl-end --
COMMENT ON TABLE public.stands IS 'All bootstrap stances (hw-labs/testing/pre-prod/prod)';
-- ddl-end --
COMMENT ON COLUMN public.stands.name IS 'Stand name';
-- ddl-end --
COMMENT ON COLUMN public.stands.cluster_map IS 'Generated cluster_map';
-- ddl-end --

-- object: instances_stands_fkey | type: CONSTRAINT --
-- ALTER TABLE public.instances DROP CONSTRAINT IF EXISTS instances_stands_fkey CASCADE;
ALTER TABLE public.instances ADD CONSTRAINT instances_stands_fkey FOREIGN KEY (stand_id)
REFERENCES public.stands (id) MATCH FULL
ON DELETE SET NULL ON UPDATE NO ACTION;
-- ddl-end --

-- object: locked_instances_locks_fkey | type: CONSTRAINT --
-- ALTER TABLE public.locked_instances DROP CONSTRAINT IF EXISTS locked_instances_locks_fkey CASCADE;
ALTER TABLE public.locked_instances ADD CONSTRAINT locked_instances_locks_fkey FOREIGN KEY (lock_id)
REFERENCES public.locks (id) MATCH FULL
ON DELETE CASCADE ON UPDATE NO ACTION;
-- ddl-end --

-- object: locked_instances_instances_fkey | type: CONSTRAINT --
-- ALTER TABLE public.locked_instances DROP CONSTRAINT IF EXISTS locked_instances_instances_fkey CASCADE;
ALTER TABLE public.locked_instances ADD CONSTRAINT locked_instances_instances_fkey FOREIGN KEY (instance_id)
REFERENCES public.instances (id) MATCH FULL
ON DELETE CASCADE ON UPDATE NO ACTION;
-- ddl-end --

-- object: hw_props_instances_fkey | type: CONSTRAINT --
-- ALTER TABLE public.hw_props DROP CONSTRAINT IF EXISTS hw_props_instances_fkey CASCADE;
ALTER TABLE public.hw_props ADD CONSTRAINT hw_props_instances_fkey FOREIGN KEY (id)
REFERENCES public.instances (id) MATCH FULL
ON DELETE CASCADE ON UPDATE NO ACTION;
-- ddl-end --

-- object: svm_props_instances_fkey | type: CONSTRAINT --
-- ALTER TABLE public.svm_props DROP CONSTRAINT IF EXISTS svm_props_instances_fkey CASCADE;
ALTER TABLE public.svm_props ADD CONSTRAINT svm_props_instances_fkey FOREIGN KEY (id)
REFERENCES public.instances (id) MATCH FULL
ON DELETE CASCADE ON UPDATE NO ACTION;
-- ddl-end --


