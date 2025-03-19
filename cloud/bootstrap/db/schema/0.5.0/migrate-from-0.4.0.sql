-- Migrage from 0.4.0 database. Written without automatic tools !!!!!

-- add new table <salt_roles> sequence
CREATE SEQUENCE public.salt_role_sq_pkey
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    START WITH 1
    CACHE 1
    NO CYCLE
    OWNED BY NONE;

-- add new table <salt_roles>
CREATE TABLE public.salt_roles(
    id integer NOT NULL DEFAULT nextval('public.salt_role_sq_pkey'::regclass),
    name text NOT NULL,
    CONSTRAINT salt_roles_pkey PRIMARY KEY (id),
    CONSTRAINT salt_roles_uq_name UNIQUE (name)
);
COMMENT ON TABLE public.salt_roles IS 'Salt roles for instances (CLOUD-36446)';

-- add new table <instance_salt_roles> sequence
CREATE SEQUENCE public.instance_salt_role_sq_pkey
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    START WITH 1
    CACHE 1
    NO CYCLE
    OWNED BY NONE;

-- add new table <instance_salt_roles>
CREATE TABLE public.instance_salt_roles(
    id integer NOT NULL DEFAULT nextval('public.instance_salt_role_sq_pkey'::regclass),
    instance_id integer NOT NULL,
    salt_role_id integer NOT NULL,
    CONSTRAINT instance_salt_roles_pkey PRIMARY KEY (id),
    CONSTRAINT instance_salt_roles_uq_instance_id_salt_role_id UNIQUE (instance_id,salt_role_id)
);
COMMENT ON TABLE public.instance_salt_roles IS 'Instance salt roles (CLOUD-36446)';

-- add new table <instance_salt_role_packages> sequence
CREATE SEQUENCE public.instance_salt_role_package_sq_pkey
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    START WITH 1
    CACHE 1
    NO CYCLE
    OWNED BY NONE;

-- add new table <instance_salt_role_packages>
CREATE TABLE public.instance_salt_role_packages(
    id integer NOT NULL DEFAULT nextval('public.instance_salt_role_package_sq_pkey'::regclass),
    instance_salt_role_id integer NOT NULL,
    package_name text NOT NULL,
    target_version text NOT NULL,
    CONSTRAINT instance_salt_role_packages_pkey PRIMARY KEY (id),
    CONSTRAINT instance_salt_roles_uq_instance_salt_role_id_package_name UNIQUE (instance_salt_role_id,package_name)
);
COMMENT ON TABLE public.instance_salt_role_packages IS 'Packages with version for instance and salt role';

-- link tables <instance_salt_roles> and <instances>
ALTER TABLE public.instance_salt_roles ADD CONSTRAINT instance_salt_roles_instances_fkey FOREIGN KEY (instance_id)
REFERENCES public.instances (id) MATCH FULL
ON DELETE CASCADE ON UPDATE NO ACTION;

-- link tables <instance_salt_roles> and <salt_roles>
ALTER TABLE public.instance_salt_roles ADD CONSTRAINT instance_salt_roles_salt_roles_fkey FOREIGN KEY (salt_role_id)
REFERENCES public.salt_roles (id) MATCH FULL
ON DELETE CASCADE ON UPDATE NO ACTION;

-- link tables <instance_salt_role_packages> and <instance_salt_roles>
ALTER TABLE public.instance_salt_role_packages ADD CONSTRAINT instance_salt_role_packages_instance_salt_roles_fkey FOREIGN KEY (instance_salt_role_id)
REFERENCES public.instance_salt_roles (id) MATCH FULL
ON DELETE CASCADE ON UPDATE NO ACTION;
