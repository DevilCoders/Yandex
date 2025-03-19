UPDATE dbaas.default_versions
SET major_version = '5.7'
WHERE component = 'xtrabackup' AND major_version = '2.4';

DO
$do$
    DECLARE
        c dbaas.clusters;
        v_rev bigint;
    BEGIN
        FOR c IN
            SELECT *
            FROM dbaas.clusters
            WHERE type = 'mysql_cluster'
            AND dbaas.visible_cluster_status(clusters.status)
            AND cid NOT IN (SELECT cid FROM dbaas.versions WHERE component = 'xtrabackup')
        LOOP

            SELECT rev INTO v_rev FROM code.lock_cluster(c.cid);

            INSERT INTO dbaas.versions(cid, component, major_version, minor_version, package_version)
            SELECT c.cid, dv.component, major_version, minor_version, package_version
            FROM dbaas.default_versions dv
            WHERE dv.env = c.env AND dv.component = 'xtrabackup' AND major_version = (
                SELECT major_version FROM dbaas.versions
                WHERE cid = c.cid AND component = 'mysql'
            );

            PERFORM code.complete_cluster_change(c.cid, v_rev);

            PERFORM code.add_finished_operation_for_current_rev(
                i_operation_id => gen_random_uuid()::text,
                i_cid => c.cid,
                i_folder_id => c.folder_id,
                i_operation_type => (c.type::text || '_modify'),
                i_metadata => '{}'::jsonb,
                i_user_id => 'migration',
                i_version => 2,
                i_hidden => true,
                i_rev => v_rev);
        END LOOP;
    END
$do$;
