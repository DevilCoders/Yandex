CREATE OR REPLACE FUNCTION code._log_minion_change(
    i_change_type deploy.minion_change_type,
    i_old_row     deploy.minions DEFAULT NULL,
    i_new_row     deploy.minions DEFAULT NULL
) RETURNS void AS $$
DECLARE
    v_minion_id bigint;
    v_old_json  jsonb;
    v_new_json  jsonb;
BEGIN
    IF (i_old_row).minion_id != (i_new_row).minion_id THEN
        RAISE EXCEPTION 'i_old_row and i_new_row have different minion_id % != %',
            (i_old_row).minion_id, (i_new_row).minion_id;
    END IF;

    v_minion_id := coalesce((i_old_row).minion_id, (i_new_row).minion_id);

    -- Cast record to json,
    -- onlyif it defined
    --
    -- record ISNULL return TRUE
    -- when all record attributes ISNULL
    IF NOT (i_old_row IS NULL) THEN
        v_old_json := to_jsonb(i_old_row);
    END IF;

    IF NOT (i_new_row IS NULL) THEN
        v_new_json := to_jsonb(i_new_row);
    END IF;

    INSERT INTO deploy.minions_change_log
        (change_type, minion_id,
         old_row, new_row)
    VALUES
        (i_change_type, v_minion_id,
         v_old_json, v_new_json);
END;
$$ LANGUAGE plpgsql;