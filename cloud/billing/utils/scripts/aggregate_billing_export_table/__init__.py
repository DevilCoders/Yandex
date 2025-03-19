def aggregate_billing_export_table(src_dir_path, dst_table_path, primary_key_fields, export_time_field,
                                   other_fields, yql_client, yt_wrapper, tmp_dir=None):
    exception_message = "Actual table is not fully exported! Try later."
    query = """
        PRAGMA yt.InferSchema = '1';
        {tmp_dir_pragma}
        $export_dir = "{export_dir}";
        $dst_table = "{dst_table}";

        $table_paths = (SELECT Path
        FROM FOLDER($export_dir, "row_count")
        WHERE Type = "table" AND Json::LookupInt64(Attributes, "row_count") > 0 ORDER BY Path DESC limit 10);

        $path_list = (SELECT AGGREGATE_LIST(Path) from $table_paths);

        $all = (SELECT {pks}, {time_field}, {other_fields} from EACH($path_list));

        $unique = (SELECT {pks}, MAX({time_field}) as {time_field} FROM $all
        GROUP BY ({pks}));

        INSERT INTO $dst_table WITH TRUNCATE
        SELECT b.* FROM $unique as a
        JOIN $all as b
        USING ({pks}, {time_field});
    """
    tmp_dir_pragma = '''
    PRAGMA yt.TmpFolder="{tmp_dir}";
    PRAGMA yt.QueryCacheMode="disable";
    PRAGMA yt.ReleaseTempData="immediate";
    PRAGMA yt.AutoMerge="disabled";'''.format(tmp_dir=tmp_dir) if tmp_dir else ""

    request = yql_client.query(query.format(
        export_dir=src_dir_path,
        pks=", ".join(["`" + field + "`" for field in primary_key_fields]),
        other_fields=", ".join(["`" + field + "`" for field in other_fields]),
        time_field="`" + export_time_field + "`",
        exception=exception_message,
        dst_table=dst_table_path,
        tmp_dir_pragma=tmp_dir_pragma
    ), syntax_version=1)
    request.run()
    result = request.get_results()
    if not result.is_success:
        errors = list(result.errors)
        if len(errors) == 1 and errors[0].format_issue().endswith(exception_message):
            return
        raise RuntimeError(
            "YQL Query is not succeeded. Operation id: {}. Errors: \n{}".format(str(request.operation_id), "\n".join(
                issue.format_issue() for issue in result.errors)))
