CREATE OR REPLACE FUNCTION code._log_master_change(
    i_change_type deploy.master_change_type,
    i_old_row     deploy.masters DEFAULT NULL,
    i_new_row     deploy.masters DEFAULT NULL
) RETURNS void AS $$
DECLARE
    v_master_id bigint;
    v_old_json  jsonb;
    v_new_json  jsonb;
BEGIN
    IF (i_old_row).master_id != (i_new_row).master_id THEN
        RAISE EXCEPTION 'i_old_row and i_new_row have different master_id % != %',
            (i_old_row).master_id, (i_new_row).master_id;
    END IF;

    v_master_id := coalesce((i_old_row).master_id, (i_new_row).master_id);

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

    INSERT INTO deploy.masters_change_log
        (change_type, master_id,
         old_row, new_row)
    VALUES
        (i_change_type, v_master_id,
         v_old_json, v_new_json);
END;
$$ LANGUAGE plpgsql;