-- {{ file }} (file this query was created with)
$chevent_log = 'logs/bs-chevent-log/{{ logs_scale }}';
$hit_log = 'logs/bs-hit-log/{{ logs_scale }}';
$nanpu_log = 'logs/janpu-event-log/{{ logs_scale }}';
$table_from = '{{ start.strftime(table_name_fmt) }}';
$table_to = '{{ end.strftime(table_name_fmt) }}';

$is_aab = ($adbbits) -> {return $adbbits is not NULL and $adbbits != '0';};

SELECT
  fielddate,
  osname,
  appid,
  version,
  sum(if(e.fraudbits='0' and $is_aab(e.adbbits), cast(e.eventcost as uint64), 0)) * 30 / 1000000 / 1.18 as aab_money,
  sum(if(e.fraudbits='0', cast(e.eventcost as uint64), 0)) * 30 / 1000000 / 1.18 as money,
  count_if(e.countertype='1' and $is_aab(e.adbbits)) as aab_shows,
  count_if(e.countertype='1') as shows,
  count_if(e.countertype='2' and $is_aab(e.adbbits)) as aab_clicks,
  count_if(e.countertype='2') as clicks,
  count_if(e.countertype='1' and e.fraudbits!='0' and $is_aab(e.adbbits)) as aab_fraud_shows,
  count_if(e.countertype='2' and e.fraudbits!='0' and $is_aab(e.adbbits)) as aab_fraud_clicks,
  count_if(e.countertype='1' and e.fraudbits!='0') as fraud_shows,
  count_if(e.countertype='2' and e.fraudbits!='0') as fraud_clicks
FROM
    (
    SELECT
        cast(hitlogid as uint64) as hitlogid,
        iso_eventtime,
        countertype,
        eventcost,
        fraudbits,
        adbbits,
        devicetype
    FROM
        RANGE($chevent_log, $table_from, $table_to)
    WHERE
        placeid in ('542', '1542')
) as e
JOIN
    (
        SELECT
            ht.hitlogid as hitlogid,
            np.OsName as OsName,
            np.AppID as AppID,
            np.SdkVersion as SdkVersion
        FROM (
            SELECT
                cast(hitlogid as uint64) as hitlogid,
                nanpureqid as NanpuReqID
            FROM
               RANGE($hit_log, $table_from, $table_to)
            WHERE nanpureqid != ''
        ) as ht JOIN (
            SELECT
                RequestID,
                OsName,
                AppID,
                SdkVersion
            FROM
                RANGE($nanpu_log, $table_from, $table_to)
            WHERE
               HandlerName == 'ad' and RequestID != ''
        ) as np ON np.RequestID = ht.NanpuReqID
    ) AS mob
USING(hitlogid)
GROUP BY
    SUBSTRING(e.iso_eventtime, 0, 15) || '0:00' as fielddate,
    mob.OsName as osname,
    mob.AppID as appid,
    mob.SdkVersion as version;
