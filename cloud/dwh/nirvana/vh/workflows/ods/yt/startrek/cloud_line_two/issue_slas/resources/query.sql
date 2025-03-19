PRAGMA Library("helpers.sql");

$src_table = {{ param["source_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};

IMPORT `helpers` SYMBOLS $lookup_int64, $lookup_string;
$get_int = ($container, $field) -> ($lookup_int64($container, $field, NULL));
$get_string = ($container, $field) -> ($lookup_string($container, $field, NULL));
$get_field = ($container, $field) -> (IF(Yson::Contains($container, $field), $container.$field, NULL));

$parse_date_string = DateTime::Parse("%Y-%m-%dT%H:%M:%S+0000");
$convert_dt = ($dt) -> (DateTime::MakeDatetime($parse_date_string(Yson::ConvertToString($dt))));
$cast_ms_to_hours = ($dt_ms) -> (DateTime::ToHours(DateTime::IntervalFromMilliseconds($dt_ms)));


$sla_root = (
    SELECT
        issues.id as issue_id,
        DictPayloads(Yson::ConvertToDict(issues.sla)) AS top_slas
    FROM $src_table as issues
    WHERE
        issues.sla IS NOT NULL 
        AND Yson::GetLength(issues.sla) > 0
);

$flatten_top_slas = (
    SELECT
        issue_id                                            AS issue_id,
        $get_int($get_field(`top_slas`, 'settings'), 'id')  AS timer_id,
        top_slas                                            AS sla_yson
    FROM $sla_root
    FLATTEN LIST BY top_slas
);

$previous_slas = (
    SELECT
        issue_id                                                    AS issue_id,
        timer_id                                                    AS timer_id,
        Yson::ConvertToList($get_field(`sla_yson`, 'previousSLAs')) AS previous_slas
    FROM $flatten_top_slas
);

$flatten_previous_slas = (
    SELECT
        issue_id            AS issue_id,
        timer_id            AS timer_id,
        previous_slas       AS sla_yson
    FROM $previous_slas
    FLATTEN LIST BY previous_slas
);

$top_sla_infos = (
    SELECT
        issue_id                                            AS issue_id,
        timer_id                                            AS timer_id,
        $get_field(`sla_yson`, 'failAt')                    AS fail_at,
        $get_int(`sla_yson`, 'failThreshold')               AS fail_threshold_ms,
        $get_int(`sla_yson`, 'pausedDuration')              AS paused_duration_ms,
        $get_int(`sla_yson`, 'spent')                       AS spent_ms,
        $get_field(`sla_yson`, 'startedAt')                 AS started_at,
        $get_string(`sla_yson`, 'violationStatus')          AS violation_status,
        $get_field(`sla_yson`, 'warnAt')                    AS warn_at,
        $get_int(`sla_yson`, 'warnThreshold')               AS warn_threshold_ms,
        $get_string(`sla_yson`, 'clockStatus')              AS clock_status
    FROM $flatten_top_slas AS slas
);

$previous_sla_infos = (
    SELECT
        issue_id                                            AS issue_id,
        timer_id                                            AS timer_id,
        $get_field(`sla_yson`, 'failAt')                    AS fail_at,
        $get_int(`sla_yson`, 'failThreshold')               AS fail_threshold_ms,
        $get_int(`sla_yson`, 'pausedDuration')              AS paused_duration_ms,
        $get_int(`sla_yson`, 'spent')                       AS spent_ms,
        $get_field(`sla_yson`, 'startedAt')                 AS started_at,
        $get_string(`sla_yson`, 'violationStatus')          AS violation_status,
        $get_field(`sla_yson`, 'warnAt')                    AS warn_at,
        $get_int(`sla_yson`, 'warnThreshold')               AS warn_threshold_ms,
        'STOPPED'                                           AS clock_status
    FROM $flatten_previous_slas AS slas
);

$all_slas = (
    SELECT *
    FROM $top_sla_infos

    UNION ALL 

    SELECT *
    FROM $previous_sla_infos
);

$convert_slas = (
    SELECT
        slas.issue_id                                                                       AS issue_id,
        slas.timer_id                                                                       AS timer_id,
        $convert_dt(slas.fail_at)                                                           AS fail_at,
        slas.fail_threshold_ms                                                              AS fail_threshold_ms,
        slas.paused_duration_ms                                                             AS paused_duration_ms,
        slas.spent_ms                                                                       AS spent_ms,
        $convert_dt(slas.started_at)                                                        AS started_at,
        slas.violation_status                                                               AS violation_status,
        $convert_dt(slas.warn_at)                                                           AS warn_at,
        slas.warn_threshold_ms                                                              AS warn_threshold_ms,
        slas.clock_status                                                                   AS clock_status,
        $cast_ms_to_hours(slas.spent_ms)                                                    AS spent_hours,
        $convert_dt(slas.started_at) + DateTime::IntervalFromMilliseconds(slas.spent_ms)    AS stopped_at
    FROM $all_slas AS slas
);

INSERT INTO $dst_table WITH TRUNCATE
    SELECT *
    FROM $convert_slas AS slas
    ORDER BY issue_id, timer_id;
