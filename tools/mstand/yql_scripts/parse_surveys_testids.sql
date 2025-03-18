use hahn;

pragma yt.PoolTrees = "physical";
pragma yt.UseDefaultTentativePoolTrees;
pragma yt.MaxRowWeight = "128M";
pragma yt.DefaultMemoryLimit = "4G";

declare $start_date as String;
declare $end_date as String;
declare $yt_pool as String;
declare $dst_path as String;

pragma yt.Pool = $yt_pool;

$datestr = DateTime::Format("%Y%m%d");
$short_start_date = String::RemoveAll($start_date, "-");
$short_end_date = String::RemoveAll($end_date, "-");

$date_range_inclusive = ($start_date_str, $end_date_str) -> {
    $start_date = cast($start_date_str as Date);
    $end_date = cast($end_date_str as Date);

    return ListCollect(
        ListMap(
            ListFromRange(0, (DateTime::ToDays($end_date - $start_date) + 1) ?? 0),
            ($x) -> (
                $datestr($start_date + DateTime::IntervalFromDays(CAST($x as Int16)))
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
    return String::ReplaceAll($str_day, "-", "");
};

$clear_list = ($lst) -> (
    ListUniq(
        ListFilter(
            ListFlatten($lst),
            ($x) -> ($x ?? "" != "")
        )
    )
);

$testids_data =
select
    String::RemoveAll(TableName(), "-") as day,
    "y" || key as key,
    Yson::ConvertToStringList(experiments) as value,
    strongestId as strongest_id,
from range(`//home/recommender/zen/sessions_aggregates/background`, $start_date, $end_date)
flatten list by (Yson::ConvertToStringList(yandexuid) as key)

union all

select
    TableName() as day,
    key,
    String::SplitToList(value, "\t") as value,
    null as strongest_id,
from range(`//home/abt/yuid_testids`, $short_start_date, $short_end_date)
with schema Struct<key: String, subkey: String, value: String>

union all

select
    $get_day(TablePath()) as day,
    "y" || yandexuid as key,
    $parse_testids(test_buckets) as value,
    null as strongest_id,
from range(`//home/videoquality/vh_analytics/strm_cube_2`, $start_date, $end_date, "sessions")
where yandexuid is not null and test_buckets is not null

union all

select
    String::RemoveAll(day, "-") as day,
    "y" || cast(yandexuid as String) as key,
    [] as value,
    strongest_id,
from `//home/goda/prod/surveys/zen_answers`
where day between $start_date and $end_date;

$yuid_whitelist =
select
    String::RemoveAll(day, "-") as day,
    "y" || cast(yandexuid as String) as key,
from `//home/goda/prod/surveys/pythia_answers`
where day between $start_date and $end_date;

$strongest_whitelist =
select
    String::RemoveAll(day, "-") as day,
    strongest_id as key,
from `//home/goda/prod/surveys/zen_answers`
where day between $start_date and $end_date;

$testids_by_yuid_raw =
select
    day,
    key,
    $clear_list(AGGREGATE_LIST(value)) as value,
    ListNotNull(AGG_LIST_DISTINCT(strongest_id)) as strongest_ids,
from $testids_data
where key != "y0"
group by day, key;

$testids_by_yuid =
select t.*
without t.strongest_ids
from $testids_by_yuid_raw as t
join any $yuid_whitelist as w
using (day, key);

$testids_by_strongest =
select
    day,
    key,
    $clear_list(AGGREGATE_LIST(value)) as value,
from (
    select day, strongest_id as key, value
    from $testids_by_yuid_raw
    flatten list by (strongest_ids as strongest_id)
) as t
join any $strongest_whitelist as w
using (day, key)
group by w.day as day, w.key as key;

evaluate for $dt in $date_range_inclusive($start_date, $end_date) do begin
    $yuid_dst = $dst_path || "/surveys_yuid_testids/" || $dt;

    insert into $yuid_dst with truncate
    select
        key,
        " surveys_yuid_testids" as subkey,
        String::JoinFromList(value, "\t") as value,
    from $testids_by_yuid
    where day = $dt and ListHasItems(value)
    order by key, subkey;

    $strongest_dst = $dst_path || "/surveys_strongest_testids/" || $dt;

    insert into $strongest_dst with truncate
    select
        key,
        " surveys_strongest_testids" as subkey,
        String::JoinFromList(value, "\t") as value,
    from $testids_by_strongest
    where day = $dt and ListHasItems(value)
    order by key, subkey;
end do;
