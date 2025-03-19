-- Migrage from 0.3.0 database. Written without automatic tools !!!!!

-- add new table <host_configs_info>
CREATE TABLE public.host_configs_info(
    id bool NOT NULL DEFAULT True,
    version integer NOT NULL,
    CONSTRAINT host_configs_info_pkey PRIMARY KEY (id),
    CONSTRAINT host_configs_info_ck_onerow CHECK (id)

);
COMMENT ON TABLE public.host_configs_info IS 'Info about host configs version (CLOUD-35467)';
COMMENT ON COLUMN public.host_configs_info.version IS 'Auto-incrementing version of host configs (CLOUD-35467)';
COMMENT ON CONSTRAINT host_configs_info_ck_onerow ON public.host_configs_info  IS 'Ensure only one row';

-- initialize table <host_configs_info> with data
INSERT INTO public.host_configs_info (id, version) VALUES (E'True', E'0');

-- add new table <stand_cluster_maps> sequence
CREATE SEQUENCE public.stand_cluster_map_sq_pkey
    INCREMENT BY 1
    MINVALUE 0
    MAXVALUE 2147483647
    START WITH 1
    CACHE 1
    NO CYCLE
    OWNED BY NONE;

-- add new table <stand_cluster_maps>
CREATE TABLE public.stand_cluster_maps(
    id integer NOT NULL DEFAULT nextval('public.stand_cluster_map_sq_pkey'::regclass),
    stand_id integer NOT NULL,
    grains jsonb,
    cluster_configs_version integer,
    yc_ci_version text,
    bootstrap_templates_version text,
    CONSTRAINT stand_cluster_map_pkey PRIMARY KEY (id),
    CONSTRAINT stand_cluster_map_uq_stand_id UNIQUE (stand_id)
);
COMMENT ON TABLE public.stand_cluster_maps IS 'Stand cluster maps (1-to-1 relation with <stands>) (CLOUD-35467)';

-- link new table <stand_cluster_maps> with <stands> table
ALTER TABLE public.stand_cluster_maps ADD CONSTRAINT stand_cluster_maps_cluster_maps_fkey FOREIGN KEY (stand_id)
REFERENCES public.stands (id) MATCH FULL
ON DELETE CASCADE ON UPDATE NO ACTION;

-- fill new table <stand_cluster_maps> with null data
DO $$ DECLARE
    r RECORD;
BEGIN
    FOR r IN (SELECT stands.id FROM stands) LOOP
        EXECUTE 'INSERT INTO stand_cluster_maps (stand_id) VALUES (' ||  r || ')';
    END LOOP;
END $$;

-- remove unneeded column from stands
ALTER TABLE stands DROP COLUMN cluster_map;
