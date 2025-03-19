-- fill entry in dbaas.versions with default_versions for given major version
CREATE OR REPLACE FUNCTION code.set_default_versions(
    i_cid           text,
    i_subcid        text,
    i_shard_id      text,
    i_ctype         dbaas.cluster_type,
    i_env           dbaas.env_type,
    i_major_version text,
    i_edition       text,
    i_rev           bigint
) RETURNS void AS $$
BEGIN

    INSERT INTO dbaas.versions (
                            cid,
                            subcid,
                            shard_id,
                            component,
                            major_version,
                            minor_version,
                            edition,
                            package_version)
    SELECT i_cid AS cid, i_subcid AS subcid, i_shard_id AS shard_id, component, major_version, minor_version, edition, package_version
        FROM dbaas.default_versions
        WHERE type=i_ctype AND major_version=i_major_version AND env=i_env AND edition=i_edition
    ON CONFLICT (cid, component) WHERE cid IS NOT NULL DO
        UPDATE SET component=EXCLUDED.component, major_version=EXCLUDED.major_version, minor_version=EXCLUDED.minor_version,
                   edition=EXCLUDED.edition, package_version=EXCLUDED.package_version;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'set_deafult_versions',
            jsonb_build_object(
                'cid', i_cid,
                'subcid', i_subcid,
                'shard_id', i_shard_id
            )
        )
    );

END;
$$ LANGUAGE plpgsql;

--temporary wrapper to support old api version
CREATE OR REPLACE FUNCTION code.set_default_versions(
    i_cid           text,
    i_subcid        text,
    i_shard_id      text,
    i_ctype         dbaas.cluster_type,
    i_env           dbaas.env_type,
    i_major_version text,
    i_rev           bigint
) RETURNS void AS $$
DECLARE
    v_major_version_1c text;
BEGIN
    v_major_version_1c := substring(i_major_version from '-1c$');
    IF v_major_version_1c IS NOT NULL THEN
        PERFORM code.set_default_versions(
            i_cid,
            i_subcid,
            i_shard_id,
            i_ctype,
            i_env,
            regexp_replace(i_major_version, '-1c$', '') ,
            '1c',
            i_rev
        );
    ELSE
        PERFORM code.set_default_versions(
            i_cid,
            i_subcid,
            i_shard_id,
            i_ctype,
            i_env,
            i_major_version,
            'default',
            i_rev
        );
    END IF;
END;
$$ LANGUAGE plpgsql;
