-- {{ file }}
$old_uids = '{{ old_uids_table }}';
$new_uids = '{{ nginx_uids_path }}/{{ last_nginx_log }}';
$old_cycada_stat = '{{ old_cycada_stat_table }}';
$new_cycada_stat = '{{ cycada_stat_path }}/{{ last_nginx_log }}';
$next_nginx_log = '//home/antiadb/bypas_uids_model/nginx_uids/next_nginx_log';
$parse = DateTime::Parse("%Y-%m-%dT%H:%M:%S");
$format = DateTime::Format("%Y-%m-%dT%H:%M:%S");
$datetime_min = DateTime::MakeDatetime($parse("{{ datetime_min }}"));

$get_device = ($device) -> {return if($device='pc', 'desktop', 'mobile');};
$nginx_log_path = '//logs/antiadb-nginx-log/stream/5min';
$from = (select _from from $next_nginx_log);
$to = '{{ last_nginx_log }}';
$match = Re2::Match("\\d+");

$service_ids_exclude_yabro = (
  'yandex.question', 'yandex_images', 'yandex_morda', 'yandex_news', 'yandex_pogoda',
  'yandex_sport', 'yandex_video', 'zen.yandex.ru', 'turbo.yandex.ru',
);

-- get records from last stream nginx log
$preselected = (
    SELECT
        if(Yson::Contains(_rest, "yandexuid"), Yson::ConvertToString(_rest["yandexuid"]), Antiadb::DecryptCrookie(Yson::ConvertToString(_rest["crookie"]))) as uid,
        Yson::ConvertToString(bamboozled['ab']) as ab,
        Yson::ConvertToString(_rest['device']) as lua_device,
        TableName() as _timestamp
    FROM RANGE($nginx_log_path, $from, $to)
    WHERE
        bamboozled is not NULL AND
        (service_id not in $service_ids_exclude_yabro or user_browser_name != 'yandex_browser') AND
        (Yson::Contains(_rest, "yandexuid") or Yson::Contains(_rest, "crookie")) AND
        Yson::Contains(_rest, "device")
);

$preselected_cycada_stat = (
    SELECT
        if(Yson::Contains(_rest, "yandexuid"), Yson::ConvertToString(_rest["yandexuid"]), Antiadb::DecryptCrookie(Yson::ConvertToString(_rest["crookie"]))) as uid,
        Yson::ConvertToString(_rest["has_cycada"]) as has_cycada,
        Yson::ConvertToString(_rest["device"]) as lua_device,
        TableName() as _timestamp
    FROM RANGE($nginx_log_path, $from, $to)
    WHERE
        bamboozled is NULL AND
        Yson::Contains(_rest, "has_cycada") AND
        (service_id not in $service_ids_exclude_yabro or user_browser_name != 'yandex_browser') AND
        (Yson::Contains(_rest, "yandexuid") or Yson::Contains(_rest, "crookie")) AND
        Yson::Contains(_rest, "device")
);

INSERT INTO $new_uids WITH TRUNCATE
    SELECT * FROM (
         -- get actual records
         SELECT
             *
         FROM $old_uids
         WHERE $datetime_min <= DateTime::MakeDatetime($parse(_timestamp))

         UNION ALL
         -- and append new records
         SELECT
             _timestamp,
             uniqid,
             device,
             COUNT_IF(ab == 'NOT_BLOCKED') as not_blocked_cnt,
             COUNT_IF(ab != 'NOT_BLOCKED' and ab != 'UNKNOWN') as blocked_cnt,
             COUNT(*) as cnt,
         FROM $preselected
         WHERE $match(uid) and lua_device in ('pc', 'smartphone') and ab is NOT NULL
         GROUP BY
             cast(uid as uint64) as uniqid,
             $get_device(lua_device) as device,
             _timestamp
     )
     ORDER BY _timestamp;

INSERT INTO $new_cycada_stat WITH TRUNCATE
    SELECT * FROM (
        -- get actual records
        SELECT
            *
        FROM $old_cycada_stat
        WHERE $datetime_min <= DateTime::MakeDatetime($parse(_timestamp))

        UNION ALL
        -- and append new records
        SELECT
            _timestamp,
            uniqid,
            device,
            COUNT_IF(has_cycada == '0') as not_cycada_cnt,
            COUNT(*) as cnt,
        FROM $preselected_cycada_stat
        WHERE $match(uid) and lua_device in ('pc', 'smartphone')
        GROUP BY
            cast(uid as uint64) as uniqid,
            $get_device(lua_device) as device,
            _timestamp
    )
    ORDER BY _timestamp;

-- make path to next stream nginx log
$_from = DateTime::FromSeconds(DateTime::ToSeconds(DateTime::MakeTimestamp($parse($to))) + CAST(300 AS Uint32));
INSERT INTO $next_nginx_log WITH TRUNCATE
    SELECT $format($_from) as _from;
COMMIT;
