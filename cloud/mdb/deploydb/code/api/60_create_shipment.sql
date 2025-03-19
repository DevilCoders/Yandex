CREATE OR REPLACE FUNCTION code.create_shipment(
    i_commands              code.command_def[],
    i_fqdns                 text[],
    i_parallel              bigint,
    i_stop_on_error_count   bigint,
    i_timeout               interval,
    i_tracing               text DEFAULT NULL,
    i_skip_fqdns            text[] DEFAULT NULL
) RETURNS code.shipment AS $$
DECLARE
    v_shipment                  deploy.shipments;
    v_shipment_commands        deploy.shipment_commands[];
    v_first_shipment_command_id bigint;
    v_fqdns                     text[];
    v_missing_fqnds             text[];
    v_deleted_fqdns             text[];
    v_commands_created          bigint;
    v_ts                        timestamptz;
    v_fqdn                      text;
    v_shipment_status           deploy.shipment_status := 'INPROGRESS'::deploy.shipment_status;
    v_command_status_parallel   deploy.command_status := 'AVAILABLE'::deploy.command_status;
    v_command_status_rest       deploy.command_status := 'BLOCKED'::deploy.command_status;
    v_done_count                bigint := 0;
    v_other_count               bigint;
BEGIN
    IF coalesce(cardinality(i_fqdns), 0) = 0 THEN
        RAISE EXCEPTION 'i_fqdns should not be empty'
            USING ERRCODE = code._error_invalid_input();
    ELSEIF coalesce(cardinality(i_commands), 0) = 0 THEN
        RAISE EXCEPTION 'i_commands should not be empty'
            USING ERRCODE = code._error_invalid_input();
    ELSEIF i_parallel > cardinality(i_fqdns) THEN
        RAISE EXCEPTION 'i_parallel should be less than or equal to i_fqdns cardinality'
            USING ERRCODE = code._error_invalid_input();
    END IF;

    v_ts := clock_timestamp();

    IF i_skip_fqdns IS NOT NULL THEN
        FOREACH v_fqdn IN ARRAY i_fqdns
        LOOP
            IF v_fqdn != ALL(i_skip_fqdns) THEN
                SELECT array_append(v_fqdns, v_fqdn) INTO v_fqdns;
            END IF;
        END LOOP;
        IF coalesce(cardinality(v_fqdns), 0) = 0 THEN
            SELECT 'DONE'::deploy.shipment_status INTO v_shipment_status;
            SELECT 'DONE'::deploy.command_status INTO v_command_status_parallel;
            SELECT 'DONE'::deploy.command_status INTO v_command_status_rest;
            SELECT i_fqdns INTO v_fqdns;
            SELECT cardinality(i_fqdns) INTO v_done_count;
        END IF;
    ELSE
        SELECT i_fqdns INTO v_fqdns;
    END IF;

    v_other_count := cardinality(v_fqdns) - v_done_count;

    INSERT INTO deploy.shipments
        (fqdns, parallel, timeout,
         stop_on_error_count, other_count,
         done_count, errors_count, total_count,
         status, created_at, updated_at,
         tracing)
    VALUES
        (v_fqdns, least(i_parallel, cardinality(v_fqdns)), i_timeout,
         least(i_stop_on_error_count, cardinality(v_fqdns)), v_other_count,
         v_done_count, 0, cardinality(v_fqdns),
         v_shipment_status, v_ts, v_ts,
         i_tracing)
    RETURNING * INTO v_shipment;

    WITH new_commands AS (
        INSERT INTO deploy.shipment_commands
            (shipment_id, type, arguments, timeout)
        SELECT
            (v_shipment).shipment_id,
            (c).type, (c).arguments, (c).timeout
          FROM unnest(i_commands) c
        RETURNING *
    )
    SELECT array_agg(sc::deploy.shipment_commands), min(shipment_command_id)
      INTO v_shipment_commands, v_first_shipment_command_id
      FROM new_commands sc;

    -- Create commands for first shipment_command_id
    INSERT INTO deploy.commands
        (shipment_command_id,
         minion_id,
         status,
         created_at, updated_at)
    SELECT
        v_first_shipment_command_id,
        minion_id,
        CASE WHEN rn <= i_parallel THEN v_command_status_parallel
                                   ELSE v_command_status_rest
        END,
        v_ts, v_ts
      FROM (
          SELECT minion_id,
                 row_number() OVER (ORDER BY minion_rn) AS rn
            FROM unnest(v_fqdns) WITH ORDINALITY AS f (minion_fqdn, minion_rn)
            JOIN deploy.minions m
              ON (m.fqdn = f.minion_fqdn AND NOT m.deleted)) ns;

    GET DIAGNOSTICS v_commands_created = ROW_COUNT;

    IF v_commands_created != cardinality(v_fqdns) THEN
        v_missing_fqnds := ARRAY(
            SELECT f
              FROM unnest(v_fqdns) AS f
             WHERE NOT EXISTS (
                 SELECT 1
                   FROM deploy.minions
                  WHERE fqdn = f
             )
        );
        v_deleted_fqdns := ARRAY(
            SELECT fqdn
              FROM deploy.minions
             WHERE fqdn = ANY(v_fqdns)
               AND deleted
        );
        RAISE EXCEPTION 'Not all commands created, got %, need %. Probably % minions missing, % minions deleted',
                  v_commands_created, cardinality(v_fqdns), v_missing_fqnds, v_deleted_fqdns
                  USING ERRCODE = code._error_invalid_state(), TABLE = 'deploy.minions';
    END IF;

    INSERT INTO deploy.commands
        (shipment_command_id,
         minion_id,
         status,
         created_at, updated_at)
    SELECT
        shipment_command_id,
        minion_id,
        v_command_status_rest,
        v_ts, v_ts
      FROM deploy.minions
     CROSS JOIN (
        SELECT shipment_command_id
          FROM unnest(v_shipment_commands)
         WHERE shipment_command_id != v_first_shipment_command_id) sc
     WHERE minions.fqdn = ANY(v_fqdns);

    RETURN code._as_shipment(v_shipment, code._as_command_def(v_shipment_commands));
END;
$$ LANGUAGE plpgsql;
