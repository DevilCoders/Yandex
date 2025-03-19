ALTER TABLE dbaas.placement_groups ADD COLUMN fqdn text;
ALTER TABLE dbaas.placement_groups ADD COLUMN local_id bigint;

ALTER TABLE dbaas.placement_groups_revs ADD COLUMN fqdn text;
ALTER TABLE dbaas.placement_groups_revs ADD COLUMN local_id bigint;

DROP INDEX IF EXISTS dbaas.uk_placement_groups_cid;
DROP INDEX IF EXISTS dbaas.uk_placement_groups_placement_group_id;

CREATE INDEX i_placement_groups_cid ON dbaas.placement_groups USING btree (cid);
CREATE INDEX i_placement_groups_placement_group_id ON dbaas.placement_groups USING btree (placement_group_id) WHERE (placement_group_id IS NOT NULL);
CREATE UNIQUE INDEX uk_placement_groups_fqdn ON dbaas.placement_groups USING btree (fqdn);
ALTER TABLE dbaas.placement_groups ADD CONSTRAINT fk_placement_groups_hostname FOREIGN KEY (fqdn) REFERENCES dbaas.hosts(fqdn) ON DELETE CASCADE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.placement_groups,
    bigint
) RETURNS dbaas.placement_groups_revs AS $$
SELECT
    $1.pg_id,
    $2,
    $1.cid,
    $1.subcid,
    $1.shard_id,
    $1.placement_group_id,
    $1.status,
    $1.fqdn, 
    $1.local_id
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.create_placement_group(
    i_cid                 text,
    i_rev                 bigint,
    i_fqdn                text,
    i_local_id            bigint
) RETURNS bigint AS $$
DECLARE v_pg_id bigint;
BEGIN
    INSERT INTO dbaas.placement_groups as pg (
        cid, status, local_id, fqdn
    )
    VALUES (
        i_cid, 'DESCRIBED'::dbaas.placement_group_status, i_local_id, i_fqdn
    )
    RETURNING pg.pg_id INTO v_pg_id;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'create_placement_group',
             jsonb_build_object(
                'cid', i_cid,
                'fqdn', i_fqdn,
                'local_id', i_local_id
            )
        )
    );

    RETURN v_pg_id;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION code.update_placement_group(
	i_cid text,
	i_placement_group_id text,
	i_rev bigint,
	i_status dbaas.placement_group_status,
    i_fqdn text
	) RETURNS void AS
$$
BEGIN
    UPDATE dbaas.placement_groups
    SET placement_group_id = i_placement_group_id,
        status             = i_status
    WHERE cid = i_cid AND fqdn = i_fqdn;

    PERFORM code.update_cluster_change(
            i_cid,
            i_rev,
            jsonb_build_object(
                    'update_placement_group',
                    jsonb_build_object(
                            'cid', i_cid,
                            'placement_group_id', i_placement_group_id,
                            'fqdn', i_fqdn
                        )
                )
        );

END;
$$ LANGUAGE plpgsql;

GRANT ALL ON FUNCTION code.create_placement_group(i_cid text, i_rev bigint, i_fqdn text, i_local_id bigint) TO backup_cli;
GRANT ALL ON FUNCTION code.create_placement_group(i_cid text, i_rev bigint, i_fqdn text, i_local_id bigint) TO cms;
GRANT ALL ON FUNCTION code.create_placement_group(i_cid text, i_rev bigint, i_fqdn text, i_local_id bigint) TO dbaas_api;
GRANT ALL ON FUNCTION code.create_placement_group(i_cid text, i_rev bigint, i_fqdn text, i_local_id bigint) TO dbaas_support;
GRANT ALL ON FUNCTION code.create_placement_group(i_cid text, i_rev bigint, i_fqdn text, i_local_id bigint) TO dbaas_worker;
GRANT ALL ON FUNCTION code.create_placement_group(i_cid text, i_rev bigint, i_fqdn text, i_local_id bigint) TO idm_service;
GRANT ALL ON FUNCTION code.create_placement_group(i_cid text, i_rev bigint, i_fqdn text, i_local_id bigint) TO katan_imp;
GRANT ALL ON FUNCTION code.create_placement_group(i_cid text, i_rev bigint, i_fqdn text, i_local_id bigint) TO logs_api;
GRANT ALL ON FUNCTION code.create_placement_group(i_cid text, i_rev bigint, i_fqdn text, i_local_id bigint) TO mdb_downtimer;
GRANT ALL ON FUNCTION code.create_placement_group(i_cid text, i_rev bigint, i_fqdn text, i_local_id bigint) TO mdb_health;
GRANT ALL ON FUNCTION code.create_placement_group(i_cid text, i_rev bigint, i_fqdn text, i_local_id bigint) TO mdb_maintenance;
GRANT ALL ON FUNCTION code.create_placement_group(i_cid text, i_rev bigint, i_fqdn text, i_local_id bigint) TO mdb_ui;
GRANT ALL ON FUNCTION code.create_placement_group(i_cid text, i_rev bigint, i_fqdn text, i_local_id bigint) TO pillar_config;
GRANT ALL ON FUNCTION code.create_placement_group(i_cid text, i_rev bigint, i_fqdn text, i_local_id bigint) TO pillar_secrets;

GRANT ALL ON FUNCTION code.update_placement_group(i_cid text, i_placement_group_id text, i_rev bigint, i_status dbaas.placement_group_status, i_fqdn text) TO backup_cli;
GRANT ALL ON FUNCTION code.update_placement_group(i_cid text, i_placement_group_id text, i_rev bigint, i_status dbaas.placement_group_status, i_fqdn text) TO cms;
GRANT ALL ON FUNCTION code.update_placement_group(i_cid text, i_placement_group_id text, i_rev bigint, i_status dbaas.placement_group_status, i_fqdn text) TO dbaas_api;
GRANT ALL ON FUNCTION code.update_placement_group(i_cid text, i_placement_group_id text, i_rev bigint, i_status dbaas.placement_group_status, i_fqdn text) TO dbaas_support;
GRANT ALL ON FUNCTION code.update_placement_group(i_cid text, i_placement_group_id text, i_rev bigint, i_status dbaas.placement_group_status, i_fqdn text) TO dbaas_worker;
GRANT ALL ON FUNCTION code.update_placement_group(i_cid text, i_placement_group_id text, i_rev bigint, i_status dbaas.placement_group_status, i_fqdn text) TO idm_service;
GRANT ALL ON FUNCTION code.update_placement_group(i_cid text, i_placement_group_id text, i_rev bigint, i_status dbaas.placement_group_status, i_fqdn text) TO katan_imp;
GRANT ALL ON FUNCTION code.update_placement_group(i_cid text, i_placement_group_id text, i_rev bigint, i_status dbaas.placement_group_status, i_fqdn text) TO logs_api;
GRANT ALL ON FUNCTION code.update_placement_group(i_cid text, i_placement_group_id text, i_rev bigint, i_status dbaas.placement_group_status, i_fqdn text) TO mdb_downtimer;
GRANT ALL ON FUNCTION code.update_placement_group(i_cid text, i_placement_group_id text, i_rev bigint, i_status dbaas.placement_group_status, i_fqdn text) TO mdb_health;
GRANT ALL ON FUNCTION code.update_placement_group(i_cid text, i_placement_group_id text, i_rev bigint, i_status dbaas.placement_group_status, i_fqdn text) TO mdb_maintenance;
GRANT ALL ON FUNCTION code.update_placement_group(i_cid text, i_placement_group_id text, i_rev bigint, i_status dbaas.placement_group_status, i_fqdn text) TO mdb_ui;
GRANT ALL ON FUNCTION code.update_placement_group(i_cid text, i_placement_group_id text, i_rev bigint, i_status dbaas.placement_group_status, i_fqdn text) TO pillar_config;
GRANT ALL ON FUNCTION code.update_placement_group(i_cid text, i_placement_group_id text, i_rev bigint, i_status dbaas.placement_group_status, i_fqdn text) TO pillar_secrets;
