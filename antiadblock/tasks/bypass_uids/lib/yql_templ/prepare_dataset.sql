-- {{ file }}
{% if stream %}
$chevent_log = 'logs/bs-chevent-log/stream/5min';
$visit_log = 'logs/visit-v2-log/stream/5min';
$visit_private_log = 'logs/visit-v2-private-log/stream/5min';
$cryprox_log = 'logs/antiadb-nginx-log/stream/5min';
$to_dict = Yson::ConvertToDict;
{% else %}
$chevent_log = 'logs/bs-chevent-log/1d';
$visit_log = 'logs/visit-v2-log/1d';
$visit_private_log = 'logs/visit-v2-private-log/1d';
$cryprox_log = 'logs/antiadb-nginx-log/1d';
$to_dict = ($x) -> { return $x; };
{% endif %}
$d1 = '{{ start.strftime(table_name_fmt) }}';
$d2 = '{{ end.strftime(table_name_fmt) }}';

$is_aab = ($adbbits) -> {return $adbbits is not NULL and $adbbits != '0';};
$desktop_name = '{{ devices.desktop }}';
$mobile_name = '{{ devices.mobile }}';
$get_device = ($devicetype) -> {return if(cast($devicetype as int64) > 4,  $desktop_name, $mobile_name);};
$get_cryprox_device = ($device) -> {return if($device='pc', $desktop_name, $mobile_name);};

$service_ids = (
  'autoru', 'kinopoisk.ru', 'yandex.question', 'yandex_afisha', 'yandex_images', 'yandex_mail',
  'yandex_morda', 'yandex_news', 'yandex_pogoda', 'yandex_realty', 'yandex_sport', 'yandex_tv',
  'yandex_video', 'zen.yandex.ru', 'turbo.yandex.ru', 'docviewer.yandex.ru',
);

-- https://bitbucket.browser.yandex-team.ru/projects/STARDUST/repos/browser-server-config/browse/common/ad_blocker.json
$service_ids_exclude_yabro = (
  'autoru', 'yandex.question', 'yandex_images', 'yandex_morda', 'yandex_news', 'yandex_pogoda',
  'yandex_sport', 'yandex_video', 'zen.yandex.ru', 'turbo.yandex.ru',
);

$aab_pages = (select cast(pageid as int64) as pageid from `{{ partners_pageids_table }}` where service_id in $service_ids);
$aab_pages_exclude_yabro = (select cast(pageid as int64) as pageid from `{{ partners_pageids_table }}` where service_id in $service_ids_exclude_yabro);

$cooked_chevent = (
  from range($chevent_log, $d1, $d2)
  select
    uniqid, countertype, device, detaileddevicetype, browsername, aab,
    count_if(cast(pageid as int64) in $aab_pages) as aab_pages_cnt,
    count(*) as cnt
  where
    fraudbits='0' AND (cast(pageid as int64) not in $aab_pages_exclude_yabro OR devicetype != '11')
  group by
    cast(uniqid as uint64) as uniqid, cast(countertype as int64) as countertype, $get_device(devicetype) as device, detaileddevicetype, browsername, $is_aab(adbbits) as aab
);

$chevent_uids = (
  select
    uniqid, device,
    max_by(detaileddevicetype, cnt) as detaileddevicetype,
    max_by(browsername, cnt) as browsername,
    SUM_IF(cnt, countertype=1) as shows,
    SUM_IF(cnt, countertype=2) as clicks,
    SUM_IF(cnt, aab and countertype=1) as aab_shows,
    SUM_IF(cnt, aab and countertype=2) as aab_clicks,
    SUM(aab_pages_cnt) as aab_pages_cnt
  from $cooked_chevent
  where uniqid is not Null and uniqid != 0
  group by uniqid, device
);

$no_aab_domain_uids = (
  SELECT
    uniqid, device, max(uniq_domains) as uniq_domains,
    SUM_IF(cnt, countertype=1) as shows,
    SUM_IF(cnt, countertype=2) as clicks
  from (
    select uniqid, device, countertype, count(distinct avd.domain) as uniq_domains, count(*) as cnt
    from
      range($chevent_log, $d1, $d2) as ch
      inner join `//home/yabs/dict/Page` as p
        on cast(p.PageID as Uint64) == cast(ch.pageid as Uint64)
      inner join `{{ no_adblock_domains_table }}` as avd
        on avd.domain == Url::CutWWW2(p.Name)
    group by cast(ch.uniqid as uint64) as uniqid, $get_device(devicetype) as device, cast(ch.countertype as int64) as countertype
  )
  where uniqid is not NULL and uniqid != 0
  group by uniqid, device
);

$cooked_cryprox = (
  SELECT uniqid, device, MAX_BY(ab, cnt) as ab, sum(cnt) as cnt
  from (
    FROM range($cryprox_log, $d1, $d2)
    SELECT uid, ab, lua_device, count(*) as cnt
    WHERE
      service_id in $service_ids and
      (service_id not in $service_ids_exclude_yabro OR user_browser_name != 'yandex_browser') AND
      bamboozled is not NULL and
      ("yandexuid" in $to_dict(_rest) or "crookie" in $to_dict(_rest)) AND
      'device' in $to_dict(_rest)
    GROUP BY
      if("yandexuid" in $to_dict(_rest), Yson::ConvertToString(_rest["yandexuid"]), Antiadb::DecryptCrookie(Yson::ConvertToString(_rest["crookie"]))) as uid,
      Yson::ConvertToString(bamboozled['ab']) as ab,
      Yson::ConvertToString(_rest['device']) as lua_device
  )
  where uid REGEXP '\\d+' and lua_device in ('pc', 'smartphone')
  group by cast(uid as uint64) as uniqid, $get_cryprox_device(lua_device) as device
);

$cryprox_uids = (
  select uniqid, device, adb, sum(cnt) as cnt
  from $cooked_cryprox
  group by uniqid, device, if(ab="NOT_BLOCKED", false, true) as adb
);

$visits_cooked = (
  SELECT
    COALESCE(MAX_BY(UserID, (VisitVersion, -Sign)), MAX_BY(ICookie, (VisitVersion, -Sign))) as uniqid,
    MAX_BY(RegionID, (VisitVersion, -Sign)) as region_id,
    MAX_BY(Sex, (VisitVersion, -Sign)) as sex,
    MAX_BY(AdBlock, (VisitVersion, -Sign)) as adblock,
    MAX_BY(Age, (VisitVersion, -Sign)) as age,
    MAX_BY(UserAgent, (VisitVersion, -Sign)) as user_agent,
    MAX_BY(IsMobile, (VisitVersion, -Sign)) as is_mobile,
    MAX_BY(OS, (VisitVersion, -Sign)) as os,
    MAX_BY(OSFamily, (VisitVersion, -Sign)) as os_familiy,
    MAX_BY(Hits, (VisitVersion, -Sign)) as hits
  FROM (select * from range($visit_log, $d1, $d2) union all select * from range($visit_private_log, $d1, $d2))
  group by VisitID
  having MAX_BY(Sign, (VisitVersion, -Sign)) = 1
);

$visit_uids = (
  SELECT
    uniqid, device,
    cast(MAX_BY(region_id, cnt) as string) as region_id,
    cast(MAX_BY(sex, cnt) as string) as sex,
    cast(MAX_BY(adblock, cnt) as string)  as adblock,
    cast(MAX_BY(age, cnt) as string) as age,
    cast(MAX_BY(user_agent, cnt) as string) as user_agent,
    cast(MAX_BY(os_familiy, cnt) as string) as os_familiy,
    sum(cnt) as cnt
  FROM (
    SELECT uniqid, is_mobile, region_id, sex, adblock, age, user_agent, os_familiy, sum(cast(hits as float)) as cnt
    FROM $visits_cooked
    where uniqid is not NULL and uniqid != 0
    group by uniqid, is_mobile, region_id, sex, adblock, age, user_agent, os_familiy
  )
  group by uniqid, if(is_mobile, $mobile_name, $desktop_name) as device
);

$result = '{{ results_path }}' || '/' || $d1 || '_' || $d2;
$dummy = 'unknown';
insert into $result WITH TRUNCATE
select
  -- chevent
  chevent.uniqid as `{{ columns.uniqid }}`,
  cast(chevent.device as string) as `{{ columns.device }}`,
  COALESCE(chevent.detaileddevicetype, $dummy) as `{{ columns.detaileddevicetype }}`,
  COALESCE(chevent.browsername, $dummy) as `{{ columns.browsername }}`,
  cast(COALESCE(chevent.shows, 0.f) as float) as `{{ columns.shows }}`,
  cast(COALESCE(chevent.clicks, 0.f) as float) as `{{ columns.clicks }}`,
  cast(COALESCE(chevent.aab_shows, 0.f) as float) as `{{ columns.aab_shows }}`,
  cast(COALESCE(chevent.aab_clicks, 0.f) as float) as `{{ columns.aab_clicks }}`,
  -- no_aab_domain_uids
  cast(COALESCE(no_aab_domain.uniq_domains, 0.f) as float) as `{{ columns.noaab_domains_unique }}`,
  cast(COALESCE(no_aab_domain.shows, 0.f) as float) as `{{ columns.noaab_domains_shows }}`,
  cast(COALESCE(no_aab_domain.clicks, 0.f) as float) as `{{ columns.noaab_domains_clicks }}`,
  -- visit_uids
  COALESCE(visit.region_id, $dummy) as `{{ columns.region_id }}`,
  COALESCE(visit.sex, $dummy) as `{{ columns.sex }}`,
  COALESCE(visit.adblock, $dummy) as `{{ columns.visit_adblock }}`,
  COALESCE(visit.age, $dummy) as `{{ columns.age }}`,
  COALESCE(visit.user_agent, $dummy) as `{{ columns.user_agent }}`,
  COALESCE(visit.os_familiy, $dummy) as `{{ columns.os_familiy }}`,
  cast(COALESCE(visit.cnt, 0.f) as float) as `{{ columns.visits_cnt }}`,
  -- chevent + visit
  cast(COALESCE(cast(chevent.shows as float) / visit.cnt, 0.f) as float) as `{{ columns.shows_ratio }}`,
  -- cryprox_uids
  cast(if(cryprox.adb=true, 1.f, 0.f) as float) as `{{ columns.adblock }}`,
from
  $chevent_uids as chevent
  left join $no_aab_domain_uids as no_aab_domain
    on chevent.uniqid=no_aab_domain.uniqid and chevent.device=no_aab_domain.device
  left join $visit_uids as visit
    on chevent.uniqid=visit.uniqid and chevent.device=visit.device
  left join $cryprox_uids as cryprox
    on chevent.uniqid=cryprox.uniqid and chevent.device=cryprox.device
{% if train %}
where chevent.aab_pages_cnt > 2
{% endif %}
order by uniqid;
commit;
