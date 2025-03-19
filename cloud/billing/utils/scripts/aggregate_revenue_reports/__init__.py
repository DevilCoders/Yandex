def aggregate_revenue_reports(src_dir_path, dst_dir_path, yql_client, yt_wrapper):
    exception_message = "Actual table is not fully exported! Try later."

    query = """
        PRAGMA yt.InferSchema = '1';
        PRAGMA yt.ExpirationInterval = '40d';
        $time_now = (SELECT CAST(CurrentUtcTimestamp() as int64) as ts);
        $export_revenue_table = "{}";
        $dst_table = "{}" || "/" || CAST($time_now as String);

        $table_paths = (SELECT Path
        FROM FOLDER($export_revenue_table, "row_count")
        WHERE Type = "table" AND Json::LookupInt64(Attributes, "row_count") > 0 ORDER BY Path DESC limit 10);

        $path_list = (SELECT AGGREGATE_LIST(Path) from $table_paths);

        $all_rows = (SELECT publisher_account_id, `date`, sku_id, billing_account_id, publisher_balance_client_id, total, created_at, export_ts
        FROM EACH($path_list));

        $unique_raws = (SELECT publisher_account_id, `date`, sku_id, billing_account_id, MAX(export_ts) as export_ts FROM $all_rows
        GROUP BY (publisher_account_id, `date`, sku_id, billing_account_id));

        $count_distinct = (SELECT count(distinct(export_ts)) from $unique_raws);

        $export_ts = (SELECT distinct(export_ts) from $unique_raws);

        DEFINE ACTION $insert_raws() AS
            INSERT INTO $dst_table SELECT * FROM $all_rows WHERE export_ts = $export_ts;
        END DEFINE;

        DEFINE ACTION $raise() AS
            SELECT ENSURE(1, 1 < 0, "{}");
        END DEFINE;

        EVALUATE IF ($count_distinct = 1)
        DO $insert_raws()
        ELSE DO $raise();
    """
    request = yql_client.query(query.format(src_dir_path, dst_dir_path, exception_message), syntax_version=1)
    request.run()
    result = request.get_results()
    if not result.is_success:
        errors = list(result.errors)
        if len(errors) == 1 and errors[0].format_issue().endswith(exception_message):
            return
        raise RuntimeError(
            "YQL Query is not succeeded. Operation id: {}. Errors: \n{}".format(str(request.operation_id), "\n".join(
                issue.format_issue() for issue in result.errors)))
