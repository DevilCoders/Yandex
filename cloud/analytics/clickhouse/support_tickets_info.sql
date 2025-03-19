CREATE TABLE %(table_name)s
(
    ticket_name String,
    pay_type String,
    components Array(String),
    all_active_time Double,
    avg_active_time Double,
    std_active_time Double,
    number_of_timer_stops Int64,
    percentile_25 Double,
    percentile_50 Double,
    percentile_75 Double,
    ticket_opened_time DateTime,
    ticket_in_work_time DateTime,
    initial_not_working_time Double,
    help_was_needed Int64,
    support Array(String),
    second_line Array(String),
    support_number Int64,
    second_line_number Int64,
    help_from_second_line Int64,
    first_summonee_time Double,
    summonee_short_groups Array(String),
    summonee_long_groups Array(String),
    help_from_not_second_line Double,
    components_hashed String,
    summonee_short_groups_hashed String
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(ticket_opened_time)
PARTITION BY toYYYYMM(ticket_opened_time)
