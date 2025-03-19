CREATE SCHEMA IF NOT EXISTS partman;

CREATE OR REPLACE FUNCTION partman.check_partitions(p_parent_table text DEFAULT NULL)
    RETURNS table(premade int, tablename text)
    LANGUAGE plpgsql SECURITY DEFINER
    AS $$
DECLARE

v_current_partition             text;
v_current_partition_id          bigint;
v_current_partition_timestamp   timestamp;
v_last_partition                text;
v_last_partition_timestamp      timestamp;
v_premade_count                 int;
v_quarter                       text;
v_row                           record;
v_sub_timestamp_max             timestamp;
v_sub_timestamp_min             timestamp;
v_tables_list_sql               text;
v_time_position                 int;
v_year                          text;

BEGIN

v_tables_list_sql := 'SELECT parent_table
                , partition_type
                , partition_interval
                , control
                , premake
                , datetime_string
                , undo_in_progress
            FROM partman.part_config';

IF p_parent_table IS NULL THEN
    v_tables_list_sql := v_tables_list_sql;
ELSE
    v_tables_list_sql := v_tables_list_sql || format(' WHERE parent_table = %L', p_parent_table);
END IF;

FOR v_row IN EXECUTE v_tables_list_sql
LOOP

    SELECT show_partitions INTO v_last_partition FROM partman.show_partitions(v_row.parent_table, 'DESC') LIMIT 1;

    IF v_row.partition_type = 'time' OR v_row.partition_type = 'time-custom' OR v_row.partition_type = 'partman' THEN

        IF v_row.partition_type = 'time' OR v_row.partition_type = 'partman' THEN
            CASE
                WHEN v_row.partition_interval::interval = '15 mins' THEN
                    v_current_partition_timestamp := date_trunc('hour', CURRENT_TIMESTAMP) +
                        '15min'::interval * floor(date_part('minute', CURRENT_TIMESTAMP) / 15.0);
                WHEN v_row.partition_interval::interval = '30 mins' THEN
                    v_current_partition_timestamp := date_trunc('hour', CURRENT_TIMESTAMP) +
                        '30min'::interval * floor(date_part('minute', CURRENT_TIMESTAMP) / 30.0);
                WHEN v_row.partition_interval::interval = '1 hour' THEN
                    v_current_partition_timestamp := date_trunc('hour', CURRENT_TIMESTAMP);
                 WHEN v_row.partition_interval::interval = '1 day' THEN
                    v_current_partition_timestamp := date_trunc('day', CURRENT_TIMESTAMP);
                WHEN v_row.partition_interval::interval = '1 week' THEN
                    v_current_partition_timestamp := date_trunc('week', CURRENT_TIMESTAMP);
                WHEN v_row.partition_interval::interval = '1 month' THEN
                    v_current_partition_timestamp := date_trunc('month', CURRENT_TIMESTAMP);
                WHEN v_row.partition_interval::interval = '3 months' THEN
                    v_current_partition_timestamp := date_trunc('quarter', CURRENT_TIMESTAMP);
                WHEN v_row.partition_interval::interval = '1 year' THEN
                    v_current_partition_timestamp := date_trunc('year', CURRENT_TIMESTAMP);
            END CASE;
        ELSIF v_row.partition_type = 'time-custom' THEN
            SELECT child_table INTO v_current_partition FROM partman.custom_time_partitions
                WHERE parent_table = v_row.parent_table AND partition_range @> CURRENT_TIMESTAMP;
            IF v_current_partition IS NULL THEN
                RAISE EXCEPTION 'Current time partition missing from custom_time_partitions config table for table % and timestamp %',
                     CURRENT_TIMESTAMP, v_row.parent_table;
            END IF;
            v_time_position := (length(v_current_partition) - position('p_' in reverse(v_current_partition))) + 2;
            v_current_partition_timestamp := to_timestamp(substring(v_current_partition from v_time_position), v_row.datetime_string);
        END IF;

        -- Determine if this table is a child of a subpartition parent. If so, get limits of what child tables can be created based on parent suffix
        SELECT sub_min::timestamp, sub_max::timestamp INTO v_sub_timestamp_min, v_sub_timestamp_max FROM partman.check_subpartition_limits(v_row.parent_table, 'time');
        -- No need to run maintenance if it's outside the bounds of the top parent.
        IF v_sub_timestamp_min IS NOT NULL THEN
            IF v_current_partition_timestamp < v_sub_timestamp_min OR v_current_partition_timestamp > v_sub_timestamp_max THEN
                CONTINUE;
            END IF;
        END IF;

        v_time_position := (length(v_last_partition) - position('p_' in reverse(v_last_partition))) + 2;
        IF v_row.partition_interval::interval <> '3 months' OR (v_row.partition_interval::interval = '3 months' AND v_row.partition_type = 'time-custom') THEN
           v_last_partition_timestamp := to_timestamp(substring(v_last_partition from v_time_position), v_row.datetime_string);
        ELSE
            -- to_timestamp doesn't recognize 'Q' date string formater. Handle it
            v_year := split_part(substring(v_last_partition from v_time_position), 'q', 1);
            v_quarter := split_part(substring(v_last_partition from v_time_position), 'q', 2);
            CASE
                WHEN v_quarter = '1' THEN
                    v_last_partition_timestamp := to_timestamp(v_year || '-01-01', 'YYYY-MM-DD');
                WHEN v_quarter = '2' THEN
                    v_last_partition_timestamp := to_timestamp(v_year || '-04-01', 'YYYY-MM-DD');
                WHEN v_quarter = '3' THEN
                    v_last_partition_timestamp := to_timestamp(v_year || '-07-01', 'YYYY-MM-DD');
                WHEN v_quarter = '4' THEN
                    v_last_partition_timestamp := to_timestamp(v_year || '-10-01', 'YYYY-MM-DD');
            END CASE;
        END IF;

        -- Check and see how many premade partitions there are.
        v_premade_count = round(EXTRACT('epoch' FROM age(v_last_partition_timestamp, v_current_partition_timestamp)) / EXTRACT('epoch' FROM v_row.partition_interval::interval));
        RETURN QUERY SELECT v_premade_count, v_row.parent_table;
    END IF;
END LOOP;
RETURN;
END
$$;

GRANT USAGE ON SCHEMA partman TO monitor;
GRANT EXECUTE ON FUNCTION partman.check_partitions(p_parent_table text) TO monitor;
