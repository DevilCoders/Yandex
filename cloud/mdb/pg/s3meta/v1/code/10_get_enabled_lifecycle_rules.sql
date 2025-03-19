CREATE OR REPLACE FUNCTION v1_code.get_enabled_lifecycle_rules(
    i_bid uuid,
    i_ts  timestamptz DEFAULT current_timestamp
) RETURNS SETOF v1_code.enabled_lifecycle_rules LANGUAGE sql STABLE  AS
$$
    SELECT bid, started_ts, finished_ts, rules
    FROM s3.enabled_lc_rules
    WHERE
        bid = i_bid
        AND started_ts <= i_ts
    ORDER BY started_ts DESC LIMIT 1
$$;
