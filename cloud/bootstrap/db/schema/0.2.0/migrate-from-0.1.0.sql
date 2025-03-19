-- Migrage from 0.1.0 database. Written without automatic tools !!!!!

-- add new table <stands> sequence
CREATE SEQUENCE public.stand_sq_pkey
    START WITH 1
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    CACHE 1;

-- add new table <stands>
CREATE TABLE public.stands (
    id integer DEFAULT nextval('public.stand_sq_pkey'::regclass) NOT NULL,
    name text NOT NULL,
    cluster_map json
);
COMMENT ON TABLE public.stands IS 'All bootstrap stances (hw-labs/testing/pre-prod/prod)';
COMMENT ON COLUMN public.stands.name IS 'Stand name';
COMMENT ON COLUMN public.stands.cluster_map IS 'Generated cluster_map';

-- add new table <stands> constraints
ALTER TABLE ONLY public.stands
    ADD CONSTRAINT stands_pkey PRIMARY KEY (id);
ALTER TABLE ONLY public.stands
    ADD CONSTRAINT stands_uq_name UNIQUE (name);

-- link table <instances> with table <stands>
ALTER TABLE ONLY public.instances
    ADD COLUMN stand_id integer;
ALTER TABLE ONLY public.instances
    ADD CONSTRAINT instances_stands_fkey FOREIGN KEY (stand_id) REFERENCES public.stands(id) MATCH FULL ON DELETE SET NULL;
