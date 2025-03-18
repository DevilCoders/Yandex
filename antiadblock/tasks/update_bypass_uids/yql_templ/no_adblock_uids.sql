-- {{ file }} (file this query was created with)
$CHEV_FROM = '{{ start.strftime(table_name_fmt) }}';
$CHEV_TO = '{{ end.strftime(table_name_fmt) }}';
$MIN_VISITED_DOMAINS = 2;
$MIN_SHOWS = 8;

$users_stat = (
  select
    uniqid,
    count(distinct avd.domain) as uniq_domains,
    count(*) as shows
  from
    range(`//cooked_logs/bs-chevent-cooked-log/1d`, $CHEV_FROM, $CHEV_TO) as ch
  inner join
    `//home/yabs/dict/Page` as p
  on
    cast(p.PageID as Uint64) == ch.pageid
  inner join
    `{{ no_adblock_domains_table }}` as avd
  on
    avd.domain == Url::CutWWW2(p.Name)
  where
    ch.countertype == 1 and ch.placeid in (542, 1542) and
    ch.devicetype > 4  -- desktops
  group by
    ch.uniqid as uniqid
);

$path = '{{ no_adblock_domains_path }}/' || cast(CurrentUtcDate() as String);
insert into $path with truncate
select uniqid from $users_stat
where uniq_domains >= $MIN_VISITED_DOMAINS and shows >= $MIN_SHOWS and uniqid > 0
order by uniqid;
