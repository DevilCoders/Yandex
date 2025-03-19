$src_folder = {{ param["src_table"] -> quote() }};
$destination_table = {{ input1 -> table_quote() }};

INSERT INTO $destination_table
    WITH TRUNCATE
SELECT
    TableName()                                                             AS event_date,
    String::RemoveFirst(Url::GetTail(url), '/')                       AS component,
    parsed_params_key2[ListIndexOf(parsed_params_key1, 'eventId')]          AS action,
    parsed_params_key2[ListIndexOf(parsed_params_key1, 'xpath')]            AS xpath,
    parsed_params_key2[ListIndexOf(parsed_params_key1, 'path')]             AS url,
    parsed_params_key2[ListIndexOf(parsed_params_key1, 'hash')]             AS path,
    hit_id,
    user_id
FROM RANGE($src_folder, `2022-03-25`)
WHERE
    counter_id = 48570998
    AND refresh IS NULL
    AND dont_count_hits = true
    AND url LIKE 'goal%'
    AND String::RemoveFirst(Url::GetTail(url), '/') IN ('yc-Link', 'yc-Yfm', 'yc-Button', 'yc-Form', 'yc-List')
    AND params != ''
    AND is_robot IS NULL
ORDER BY
    event_date,
    component,
    action,
    url,
    xpath,
    path
;
