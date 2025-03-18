-- {{ file }} (file this query was created with)
$is_aab = ($ttag) -> {return (CAST($ttag as UInt64) & (1ul << 49)) != 0;};
$CHEV_FROM = '{{ start.strftime(table_name_fmt) }}';
$CHEV_TO = '{{ end.strftime(table_name_fmt) }}';
$MIN_USERS_PER_DAY = 250;
$MAX_ADB_SHOWS_RATIO = 0.05;
$MAX_ADB_USERS_RATIO = 0.05;
$MAX_AAB_SHOWS_RATIO = 0.01;
$MAX_AAB_CLICKS_RATIO = 0.01;
$domain_stat = (
    select
        domain,
        is_adb_user,
        count_if(ch.countertype == 1) as shows,
        count_if(ch.countertype == 2) as clicks,
        count(distinct ch.uniqid) as uniq_users,
        count_if((ch.countertype == 1) and $is_aab(ch.testtag)) as aab_shows,
        count_if((ch.countertype == 2) and $is_aab(ch.testtag)) as aab_clicks
    from
        range(`//cooked_logs/bs-chevent-cooked-log/1d`, $CHEV_FROM, $CHEV_TO) as ch
    inner join
        `//home/yabs/dict/Page` as p
    on
        cast(p.PageID as Uint64) == ch.pageid
    inner join
        `{{ uds_state_table }}` as u -- users from Yandex Bro with a determined adblock-usage
    on
        cast(u.uid as Uint64) == ch.uniqid
    where
        ch.fraudbits == 0 and ch.placeid in (542, 1542) and
        ch.devicetype > 4 and  -- desktops
        ch.browsername == 'YandexBrowser'
    group by
        Url::CutWWW2(p.Name) as domain,
        u.adb as is_adb_user
);
$aggregated_stat = (
    select
        domain,
        cast(sum_if(shows, is_adb_user) as Double) / sum(shows) as adb_shows_ratio,
        cast(sum_if(clicks, is_adb_user) as Double) / sum(clicks) as adb_clicks_ratio,
        cast(sum_if(uniq_users, is_adb_user) as Double) / sum(uniq_users) as adb_users_ratio,
        cast(sum(aab_shows) as Double) / sum(shows) as aab_shows_ratio,
        cast(sum(aab_clicks) as Double) / sum(clicks) as aab_clicks_ratio,
        sum(shows) as shows,
        sum(clicks) as clicks,
        sum(uniq_users) as users,
        sum_if(shows, is_adb_user) as adb_shows,
        sum_if(clicks, is_adb_user) as adb_clicks,
        sum_if(uniq_users, is_adb_user) as adb_users
    from
        $domain_stat
    group by
        domain
);
$path = '{{ no_adblock_domains_path }}/' || cast(CurrentUtcDate() as String);
insert into $path with truncate
select * from $aggregated_stat
where
    users >= $MIN_USERS_PER_DAY
    and adb_users_ratio <= $MAX_ADB_USERS_RATIO
    and adb_shows_ratio <= $MAX_ADB_SHOWS_RATIO
    and aab_shows_ratio <= $MAX_AAB_SHOWS_RATIO
    and aab_clicks_ratio <= $MAX_AAB_CLICKS_RATIO
;