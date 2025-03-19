use hahn;
PRAGMA AnsiInForEmptyOrNullableItemsCollections;

DEFINE SUBQUERY $l2_statuses() AS
    $getField = ($el) -> {RETURN Yson::YPathString($el, '/field')};
    $fieldsNewValues = ($el) -> { RETURN Yson::YPath($el, '/newValue/value')};
    $fieldsOldValues = ($el) -> { RETURN Yson::YPath($el, '/oldValue/value')};

    $get_data = ($yson)->{RETURN IF(Yson::Contains($yson,'pay'), Yson::LookupString($yson, 'pay'),  'no_pay_type')};
    $unify_pay_type = ($str) -> {RETURN
                            (CASE
                                WHEN $str IN ('standart', 'standard', 'Standard') THEN 'standard'
                                WHEN $str IN ('free', ' free') THEN 'free'
                                WHEN $str='' THEN 'no_pay_type'
                                ELSE $str
                            END)};


    $statuses_names = (
        SELECT 
            id,
            key,
            Yson::ConvertToString(Yson::SerializeText(name.RUSSIAN)) AS statusRu
        FROM `//home/startrek/tables/prod/yandex-team/common/statuses`
        );

    $issue_events = (
        SELECT 
            issues.key AS key,
            issues.created AS created,
            $unify_pay_type($get_data(issues.customFields)) AS pay,
            issues.id AS issue_id,
            issue_events.id AS event_id,
            issue_events.`date` AS event_date,
            issue_events.changes AS changes,
            ListZip(ListMap(Yson::ConvertToList(issue_events.changes), $getField),
                    ListMap(Yson::ConvertToList(issue_events.changes), $fieldsNewValues),
                    ListMap(Yson::ConvertToList(issue_events.changes), $fieldsOldValues)) AS fields,
        FROM  `//home/startrek/tables/prod/yandex-team/queue/CLOUDSUPPORT/issue_events` AS issue_events
        INNER JOIN `//home/startrek/tables/prod/yandex-team/queue/CLOUDSUPPORT/issues` AS issues ON issues.id=issue_events.issue
    );

    $statuses = (
        SELECT 
            key,
            created,
            pay,
            issue_id,
            event_id,
            event_date,
            field,
            new_value,
            old_value
        FROM (
            SELECT
                key,
                created,
                pay,
                issue_id,
                event_id,
                event_date,
                fields.0 AS field,
                CASE 
                    WHEN fields.0='status' THEN Yson::ConvertToString(Yson::Lookup(fields.1, 'id'))
                    ELSE Yson::ConvertToString(fields.1)
                END AS new_value,
                CASE 
                    WHEN fields.0='status' THEN Yson::ConvertToString(Yson::Lookup(fields.2, 'id'))
                    ELSE Yson::ConvertToString(fields.2)
                END AS old_value,
            FROM $issue_events AS issue_events
            FLATTEN list by fields)
    WHERE field in ('status', 'statusStartTime' )
        );
    
    $to_seconds = ($str) -> {RETURN DateTime::ToSeconds(DateTime::MakeDatetime(DateTime::Parse('%Y-%m-%dT%H:%M:%S')(SUBSTRING($str, 0, 23))))};

    SELECT 
        t1.key AS key,
        CAST(Math::Round(t1.created/1000) AS Uint32) AS created,
        t1.pay AS pay_type,
        t1.issue_id AS issue_id,
        t1.event_id AS event_id,
        CAST(Math::Round(t1.event_date/1000) AS Uint32) AS event_date,
        t1.new_value AS status_new,
        status_names1.statusRu AS status_ru_new,
        t1.old_value AS status_old,
        status_names2.statusRu AS status_ru_old,
        $to_seconds(t2.new_value) AS status_old_time_end,
        $to_seconds(t2.old_value) AS status_old_time_start,
    FROM $statuses AS t1
    LEFT JOIN $statuses AS t2 ON t1.event_id=t2.event_id
    LEFT JOIN $statuses_names AS status_names1  ON status_names1.id = t1.new_value
    LEFT JOIN $statuses_names AS status_names2  ON status_names2.id = t1.old_value
    WHERE 
        t1.field ='status'
        AND t2.field !='status';
    
END DEFINE;


 
DEFINE SUBQUERY $l2_comments() AS
        SELECT 
            issues.key AS issue_key,
            issues.id AS issue_id,
            comments.id AS comment_id,
            staff.login AS login,
            comments.created AS created,
            IF(staff.quit_at IS NULL, 1, 0) AS is_working
        FROM `//home/startrek/tables/prod/yandex-team/queue/CLOUDSUPPORT/comments` AS comments
        INNER JOIN `//home/startrek/tables/prod/yandex-team/queue/CLOUDSUPPORT/issues` AS issues ON issues.id = comments.issue
        INNER JOIN `//home/startrek/tables/prod/yandex-team/common/users` AS users ON users.uid = comments.author
        INNER JOIN  `//home/cloud_analytics/import/staff/cloud_staff/cloud_staff` AS staff ON staff.login = users.login
        WHERE staff.group_name = "Группа ведущих инженеров поддержки Яндекс.Облака"
END DEFINE;

export $l2_comments, $l2_statuses;