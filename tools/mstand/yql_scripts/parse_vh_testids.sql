use hahn;

pragma yt.PoolTrees = "physical";
pragma yt.UseDefaultTentativePoolTrees;
pragma yt.MaxRowWeight = "64M";

declare $start_date as String;
declare $end_date as String;
declare $yt_pool as String;
declare $dst_path as String;

PRAGMA yt.Pool = $yt_pool;

$datestr = DateTime::Format("%Y%m%d");

$date_range_inclusive = ($start_date_str, $end_date_str) -> {
    $start_date = cast($start_date_str as Date);
    $end_date = cast($end_date_str as Date);

    return ListCollect(
        ListMap(
            ListFromRange(0, (DateTime::ToDays($end_date-$start_date) + 1) ?? 0),
            ($x) -> (
                $start_date + DateTime::IntervalFromDays(CAST($x as Int16))
            )
        )
    );
};

$parse_testids = ($test_buckets) -> (
    ListMap(
        String::SplitToList($test_buckets, ";"),
        ($x) -> (ListHead(String::SplitToList($x, ",")))
    )
);

$get_day = ($table_path) -> {
    $str_day = ListReverse(String::SplitToList($table_path, "/"))[1];
    return cast($str_day as Date);
};

$testids_stream = (
    select
        day,
        key,
        " yuid_testids" as subkey,
        String::JoinFromList(
            ListUniq(
                ListFlatten(
                    AGGREGATE_LIST(
                        $parse_testids(test_buckets)
                    )
                )
            ),
            "\t"
        ) as value,
    from range(`//home/videoquality/vh_analytics/strm_cube_2`, $start_date, $end_date, "sessions")
    where yandexuid is not null and test_buckets is not null
    group by $get_day(TablePath()) as day, "y" || yandexuid as key
);

evaluate for $dt in $date_range_inclusive($start_date, $end_date) do begin
    $dst = $dst_path || "/" || $datestr($dt);

    insert into $dst with truncate
    select key, subkey, value
    from $testids_stream
    where day = $dt
    order by key, subkey;
end do;
