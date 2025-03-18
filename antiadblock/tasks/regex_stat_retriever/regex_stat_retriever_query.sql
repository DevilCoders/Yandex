$table_paths = (
    SELECT AGGREGATE_LIST(Path) FROM (
        SELECT Path
        FROM FOLDER("{{yt_logs_path }}", "schema;row_count")
        WHERE
            Type = "table" AND
            Yson::GetLength(Attributes.schema) > 0 AND
            Yson::LookupInt64(Attributes, "row_count") > 0
        ORDER BY Path DESC
        LIMIT {{ tables_to_use }}
        )
    );

SELECT
    service_id,
    regex,
    AGGREGATE_LIST_DISTINCT(url) as urls
from EACH($table_paths)
where action="regex_stat"
GROUP BY service_id, Yson::ConvertToString(_rest["regex"]) as regex
ORDER BY service_id;
