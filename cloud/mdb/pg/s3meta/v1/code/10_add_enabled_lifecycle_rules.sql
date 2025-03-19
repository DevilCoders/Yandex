CREATE OR REPLACE FUNCTION v1_code.add_enabled_lifecycle_rules(
    i_bid uuid,
    i_rules JSONB
) RETURNS v1_code.enabled_lifecycle_rules LANGUAGE plpgsql AS
$$
DECLARE
    v_current_ts timestamptz := current_timestamp;
    v_last_rules v1_code.enabled_lifecycle_rules;
    result v1_code.enabled_lifecycle_rules;
BEGIN
    SELECT bid, started_ts, finished_ts, rules
    INTO v_last_rules
    FROM s3.enabled_lc_rules
    WHERE
        bid = i_bid
        AND started_ts <= v_current_ts
    ORDER BY started_ts DESC LIMIT 1
    FOR UPDATE;

    IF FOUND THEN
        UPDATE s3.enabled_lc_rules SET finished_ts = v_current_ts
        WHERE bid = v_last_rules.bid AND started_ts = v_last_rules.started_ts;
    END IF;

    INSERT INTO s3.enabled_lc_rules (
        bid, started_ts, finished_ts, rules
    ) VALUES (i_bid, v_current_ts, 'infinity'::timestamptz, i_rules)
    RETURNING enabled_lc_rules.bid,
        enabled_lc_rules.started_ts,
        enabled_lc_rules.finished_ts,
        enabled_lc_rules.rules
        INTO result;
    return result;
END;
$$;
