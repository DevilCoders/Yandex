DO
$do$
    DECLARE
        c dbaas.clusters;
        v_rev bigint;
    BEGIN
        FOR c IN
            SELECT *
            FROM dbaas.clusters
            WHERE type = 'postgresql_cluster'
              AND (status = 'RUNNING' OR status = 'STOPPED')
              AND EXISTS(
                    SELECT 1
                    FROM dbaas.pillar
                    WHERE clusters.cid = pillar.cid
                      AND value @> '{"data": {"use_1c": true}}')
            LOOP
                v_rev := (code.lock_cluster(c.cid)).rev;
                UPDATE dbaas.pillar SET value = value #- '{data,use_1c}' WHERE cid = c.cid;
                PERFORM code.complete_cluster_change(c.cid, v_rev);
            END LOOP;
    END
$do$;
