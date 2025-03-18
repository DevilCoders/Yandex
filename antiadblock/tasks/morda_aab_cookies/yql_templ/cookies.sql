-- {{ file }} (file this query was created with)
$d1 = '{{ start.strftime(table_name_fmt) }}';
$d2 = '{{ end.strftime(table_name_fmt) }}';
$log = '{{ logs_base_path }}/{{ logs_scale }}';

$yandexuid = Pire::Capture(".*yandexuid=(\\d+);.*");
$aab_cookies = Re2::FindAndConsume('({{ "|".join(cookies) }})=');

$grouped_by_yandexuid = (
  FROM range($log, $d1, $d2)
  select
    time, yandexuid, device,
    max($aab_cookies(WeakField(`cookies`, "String"))) as cookies,
    count(*) as cnt
  where WeakField(`cookies`, "String") is not null
  group by
    COALESCE(WeakField(`yandexuid`, "String"), WeakField(`icookie`, "String"), $yandexuid(WeakField(`cookies`, "String"))) as yandexuid,
    if(WeakField(`ua.ismobile`, "String")='1' or WeakField(`ua.istablet`, "String")='1', 'mobile', 'desktop') as device,
    String::Substring(`timestamp`, 0, 13)||':00:00' as time
);

-- table #1: cookie of the day vs blstr
from $grouped_by_yandexuid
select
  time, device,
  count(*) as users,
  count_if(ListLength(`cookies`) = 1 and `cookies`[0] = 'bltsr') as bltsr_users,
  count_if(ListLength(`cookies`) > 0) as adb_users,
  sum(cnt) as requests,
  sum_if(cnt, ListLength(`cookies`) = 1 and `cookies`[0] = 'bltsr') as bltsr_requests,
  sum_if(cnt, ListLength(`cookies`) > 0) as adb_requests
where yandexuid is not NULL
group by time, device;

-- table #2: count all AAB cookies stats separately
select time, device, cookie, count(*) as users, sum(cnt) as requests
from (
  select time, device, yandexuid, cookie, cnt
  from $grouped_by_yandexuid
  flatten by cookies as cookie
  where yandexuid is not NULL
  union all
  select time, device, yandexuid, '{{ not_adb_name }}' as cookie, cnt
  from $grouped_by_yandexuid
  where yandexuid is not NULL and ListLength(cookies)=0
)
group by time, device, cookie
