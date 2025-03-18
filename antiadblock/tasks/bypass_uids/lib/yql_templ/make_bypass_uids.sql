-- {{ file }}
$nginx_uid_table = '{{ nginx_uid_table_path }}';
$cycada_stat_table = '{{ cycada_stat_table_path }}';
$predict_uid_table = '{{ predict_uid_table_path }}';
$predict_uid_stream_table = '{{ predict_uid_stream_table_path }}';
$result_table = '{{ result_table }}';
$device = '{{ device }}';

$ratio = {{ ratio }};
$min_cnt = {{ min_cnt }};

$get_status = ($not_blocked_cnt, $blocked_cnt, $cnt) -> {return if($not_blocked_cnt/$cnt > $ratio, "not_blocked", (if($blocked_cnt/$cnt > $ratio, "blocked", NULL)));};

$preselected_nginx_uid = (
    SELECT
        uniqid,
        $get_status(not_blocked_cnt, blocked_cnt, cnt) as status
    FROM (
        SELECT
            uniqid,
            SUM(cnt) as cnt,
            SUM(blocked_cnt) as blocked_cnt,
            SUM(not_blocked_cnt) as not_blocked_cnt,
        FROM $nginx_uid_table
        WHERE
            device = $device
        GROUP BY uniqid
    )
    WHERE cnt > $min_cnt
);

$preselected_cycada_stat = (
    SELECT
        uniqid
    FROM (
        SELECT
            uniqid,
            SUM(cnt) as cnt,
            SUM(not_cycada_cnt) as not_cycada_cnt,
        FROM $cycada_stat_table
        WHERE
            device = $device
        GROUP BY uniqid
    )
    WHERE cnt > $min_cnt and not_cycada_cnt / cnt > $ratio
);

$not_blocked_uids = (
    SELECT uniqid
    FROM $preselected_nginx_uid
    WHERE status = 'not_blocked'
);

$blocked_uids = (
    SELECT uniqid
    FROM $preselected_nginx_uid
    WHERE status = 'blocked'
    {% if remove_uids_without_cycada %}
    UNION ALL
    SELECT uniqid
    FROM $preselected_cycada_stat
    {% endif %}
);

$predict_uid = (
    {% if use_stream_predicts %}
    SELECT DISTINCT uniqid
    FROM CONCAT($predict_uid_table, $predict_uid_stream_table)
    {% else %}
    SELECT uniqid
    FROM $predict_uid_table
    {% endif %}
    {% if remove_blocked_uids %}
    WHERE uniqid not in $blocked_uids
    {% endif %}
);

INSERT INTO $result_table WITH TRUNCATE
    SELECT DISTINCT uniqid
    {% if use_not_blocked_uids and use_predict_uids %}
    FROM (
        SELECT uniqid FROM $predict_uid
        UNION all
        SELECT uniqid FROM $not_blocked_uids
    )
    {% elif use_not_blocked_uids %}
    FROM $not_blocked_uids
    {% else %}
    FROM $predict_uid
    {% endif %}
    ORDER BY uniqid;
COMMIT;
