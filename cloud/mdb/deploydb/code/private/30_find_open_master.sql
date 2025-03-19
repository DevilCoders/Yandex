CREATE OR REPLACE FUNCTION code._find_open_master(
    i_group deploy.groups
) RETURNS deploy.masters AS $$
DECLARE
    v_open_master_id bigint;
BEGIN
    SELECT master_id
      INTO v_open_master_id
      FROM deploy.masters
      LEFT JOIN (
        SELECT master_id,
                count(*) AS minions_count
          FROM deploy.minions
         WHERE deleted = false
         GROUP BY master_id) c
     USING (master_id)
     WHERE is_open AND is_alive AND group_id = i_group.group_id
     ORDER BY minions_count ASC NULLS FIRST,
              master_id ASC
     LIMIT 1;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find open master in group %', (i_group).name
              USING ERRCODE = code._error_no_open_master(), TABLE = 'deploy.masters';
    END IF;

    RETURN (SELECT masters
              FROM deploy.masters
             WHERE master_id = v_open_master_id);
END;
$$ LANGUAGE plpgsql STABLE;
