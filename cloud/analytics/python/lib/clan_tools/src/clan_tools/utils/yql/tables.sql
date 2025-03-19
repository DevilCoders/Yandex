USE hahn;

DEFINE SUBQUERY $last_non_empty_table($path) AS
    $max_path = (
        SELECT MAX(Path) AS Path
        FROM FOLDER($path, "row_count")
        WHERE Type = "table"
            AND Yson::LookupInt64(Attributes, "row_count") > 0
    );
    SELECT * FROM CONCAT($max_path);
END DEFINE;

DEFINE SUBQUERY $last_n_tables($path, $n) AS
    $table_paths = (
        SELECT ListTake(ListSortDesc(AGGREGATE_LIST(Path)), $n)
        FROM FOLDER($path, "schema;row_count")
        WHERE
            Type = "table" AND
            Yson::LookupInt64(Attributes, "row_count") > 0
    );
    SELECT * FROM each($table_paths);
END DEFINE;

EXPORT $last_non_empty_table, $last_n_tables;