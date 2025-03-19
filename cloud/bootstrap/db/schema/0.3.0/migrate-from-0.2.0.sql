-- Migrage from 0.2.0 database. Written without automatic tools !!!!!

-- add new table <instance_groups> sequence
CREATE SEQUENCE public.instance_group_sq_pkey
    START WITH 1
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    CACHE 1;

-- add new table <instance_groups>
CREATE TABLE public.instance_groups (
    id integer DEFAULT nextval('public.instance_group_sq_pkey'::regclass) NOT NULL,
    name text NOT NULL,
    stand_id integer NOT NULL
);
COMMENT ON TABLE public.instance_groups IS 'Instance groups attached to stands (CLOUD-33565)';

-- add new table <instance_groups> constraints
ALTER TABLE ONLY public.instance_groups
    ADD CONSTRAINT instance_groups_pkey PRIMARY KEY (id);
ALTER TABLE ONLY public.instance_groups
    ADD CONSTRAINT instance_groups_uq_name_stand_id UNIQUE (name, stand_id);

-- link new table <instanc_groups> with table <stands>
ALTER TABLE ONLY public.instance_groups
    ADD CONSTRAINT instance_groups_stands_fkey FOREIGN KEY (stand_id) REFERENCES public.stands(id) MATCH FULL ON DELETE CASCADE;

-- add new table <instance_group_releases> sequence
CREATE SEQUENCE public.instance_group_release_sq_pkey
    START WITH 1
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    CACHE 1;

-- add new table <intance_groups_releases>
CREATE TABLE public.instance_group_releases (
    id integer DEFAULT nextval('public.instance_group_release_sq_pkey'::regclass) NOT NULL,
    url text,
    image_id text,
    instance_group_id integer NOT NULL
);
COMMENT ON TABLE public.instance_group_releases IS 'Releses (images) for instance groups (currently 1:1 relation to <instance_groups> table) (CLOUD-33565)';

-- add new table <instance_group_releases> constraints
ALTER TABLE ONLY public.instance_group_releases
    ADD CONSTRAINT instance_group_release_pkey PRIMARY KEY (id);
ALTER TABLE ONLY public.instance_group_releases
    ADD CONSTRAINT instance_group_release_uq_instance_group_id UNIQUE (instance_group_id);
COMMENT ON CONSTRAINT instance_group_release_uq_instance_group_id ON public.instance_group_releases IS '1:1 relation to <instance_groups> table';

-- link new table <instance_group_releases> with new table <instance_groups>
ALTER TABLE ONLY public.instance_group_releases
    ADD CONSTRAINT instance_group_releases_instance_groups_fkey FOREIGN KEY (instance_group_id) REFERENCES public.instance_groups(id) MATCH FULL ON DELETE CASCADE;
