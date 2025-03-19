CREATE OR REPLACE FUNCTION code.get_unmanaged_config(
    i_fqdn      text,
    i_target_id text   DEFAULT NULL,
    i_rev       bigint DEFAULT NULL
) RETURNS jsonb AS $$
DECLARE
    v_rev            bigint;
    v_config         jsonb[];
BEGIN
    IF i_rev IS NULL THEN
        SELECT c.actual_rev
          INTO v_rev
          FROM dbaas.clusters c
               JOIN dbaas.subclusters sc USING (cid)
               JOIN dbaas.hosts h USING (subcid)
         WHERE h.fqdn = i_fqdn
               AND code.visible(c)
               AND NOT code.managed(c);

        IF NOT found THEN
            RAISE EXCEPTION 'Unable to find unmanaged cluster by host %', i_fqdn
            USING ERRCODE = 'MDB01';
        END IF;
    ELSE
        v_rev := i_rev;
    END IF;

    SELECT array(
        SELECT value FROM code.get_rev_pillar_by_host(i_fqdn, v_rev, i_target_id) ORDER BY priority)
    INTO v_config;

    RETURN code.combine_dict(v_config);
END;
$$ LANGUAGE plpgsql STABLE;
