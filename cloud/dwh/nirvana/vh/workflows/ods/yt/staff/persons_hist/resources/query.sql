PRAGMA Library("helpers.sql");

IMPORT `helpers` SYMBOLS $get_md5;

$src_table = {{ param["source_path"] -> quote() }};
$pii_src_table = {{ param["pii_source_path"] -> quote() }};

$dst_table = {{ input1 -> table_quote() }};
$pii_dst_table = {{ param["pii_destination_path"] -> quote() }};

-- all period
$start_ts = CAST(Date('2017-01-01') as DateTime);
$end_ts = CAST(Date('2099-12-31') as DateTime);

-- current load
$end_current_ts = CAST(CurrentUtcTimestamp() as DateTime);
$start_current_ts = $end_current_ts + DateTime::IntervalFromSeconds(1);

-- old_data = if desc not exist then $src_table_raw
$tablepath_re = Re2::Capture("^(.+)/[^/]+$");
$table_path = ($str) -> (
    $tablepath_re($str)._1
);


$dst_path = $table_path($dst_table);
$pii_dst_path = $table_path($pii_dst_table);

$exist = SELECT ListHas( AGGREGATE_LIST( TableName(Path)), TableName($dst_table)) FROM FOLDER($dst_path);
$exist = EvaluateExpr($exist);

$pii_exist = SELECT ListHas( AGGREGATE_LIST( TableName(Path)), TableName($pii_dst_table)) FROM FOLDER($pii_dst_path);
$pii_exist = EvaluateExpr($pii_exist);


DEFINE SUBQUERY $src_data($is_insert,$source) AS
    SELECT
        a.*,
        $get_md5(ForceRemoveMember(TableRow(),'staff_user_hash'))   AS check_sum,
        if($is_insert,$start_ts,$start_current_ts)                  AS start_ts,
        $end_ts                                                     AS end_ts
    from $source as a;
END DEFINE;


DEFINE SUBQUERY $merge_data($source,$destination) AS

    $new_data =(
    SELECT a.* FROM $src_data(False,$source) AS a
    );

    $old_data = (
    SELECT a.* FROM $destination AS a
    );

    -- hist data end_ts < $end_ts
    SELECT
        *
    FROM $old_data AS old
    WHERE old.end_ts != $end_ts
    UNION ALL
    -- hist data end_ts = $end_ts
    SELECT
        old.*,
        CASE    WHEN new_data_short.staff_user_hash IS Null THEN old.end_ts
                WHEN new.staff_user_hash IS Null THEN $end_current_ts
                ELSE old.end_ts  END   AS end_ts
    WITHOUT
        old.end_ts
    FROM $old_data AS old
    LEFT JOIN $new_data AS new_data_short
            ON old.staff_user_hash=new_data_short.staff_user_hash AND old.end_ts=new_data_short.end_ts
    LEFT JOIN $new_data AS new
            ON old.staff_user_hash=new.staff_user_hash AND old.end_ts=new.end_ts  AND old.check_sum=new.check_sum
    WHERE old.end_ts = $end_ts
    -- new data
    UNION ALL
    SELECT
        new.*
    FROM
        $new_data AS new
    LEFT JOIN $old_data AS old
        ON old.staff_user_hash=new.staff_user_hash AND old.end_ts=new.end_ts  AND old.check_sum=new.check_sum
    WHERE old.staff_user_hash IS Null
    ;
END DEFINE;


EVALUATE IF $exist
DO BEGIN
    INSERT INTO $dst_table WITH TRUNCATE
    PROCESS $merge_data($src_table,$dst_table)
END DO
    ELSE
DO BEGIN
    INSERT INTO $dst_table WITH TRUNCATE
    PROCESS $src_data(True,$src_table)
END DO;


EVALUATE IF $pii_exist
DO BEGIN
    INSERT INTO $pii_dst_table WITH TRUNCATE
    PROCESS $merge_data($pii_src_table,$pii_dst_table)
END DO
    ELSE
DO BEGIN
    INSERT INTO $pii_dst_table WITH TRUNCATE
    PROCESS $src_data(True,$pii_src_table)
END DO;
;
