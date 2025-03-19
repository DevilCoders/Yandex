def find_duplicate_metrics(src_dir_path, yql_client, yt_wrapper, since_date=None):
    query = """
    PRAGMA yt.InferSchema = '1';
    $billing_metrics_dir = "{src_dir}";
    $table_paths = (SELECT Path FROM FOLDER($billing_metrics_dir, "row_count;key")
    WHERE Type = "table" AND Json::LookupInt64(Attributes, "row_count") > 0
    ORDER BY Path DESC LIMIT 2);
    $path_list = (SELECT AGGREGATE_LIST(Path) from $table_paths);
    $r = (SELECT
        id, `schema`,
        COUNT(*) AS cnt,
        AGGREGATE_LIST_DISTINCT(cloud_id) AS distinct_cloud_ids,
        AGGREGATE_LIST_DISTINCT(folder_id) AS distinct_folder_ids,
        AGGREGATE_LIST(source_wt) AS source_wts,
        AGGREGATE_LIST_DISTINCT(resource_id) AS distinct_resource_ids,
        AGGREGATE_LIST_DISTINCT(source_uri) AS distinct_source_uris,
        AGGREGATE_LIST_DISTINCT(source_id) AS distinct_source_ids,
        AGGREGATE_LIST_DISTINCT(_stbx) AS distinct_stbxs,
        SUM(Yson::ConvertToDouble(Yson::Lookup(usage, "quantity"), Yson::Options(True, False))) AS total_quantity

    FROM EACH($path_list) GROUP BY (id, `schema`));
    SELECT * FROM $r WHERE cnt > 1;
    """

    if since_date:
        query = query.format(src_dir=src_dir_path,
                             additional_part='AND Json::LookupString(Attributes, "key") >= "{}"'.format(since_date))
    else:
        query = query.format(src_dir=src_dir_path, additional_part='ORDER BY Path DESC LIMIT 2')

    request = yql_client.query(query, syntax_version=1)
    request.run()
    result = request.get_results()
    if not result.is_success:
        raise RuntimeError(
            "YQL Query is not succeeded. Operation id: {}. Errors: \n{}".format(str(request.operation_id), "\n".join(
                issue.format_issue() for issue in result.errors)))
    res = []
    for table in result:
        for row in table.rows:
            line = {}
            for cell, name in zip(row, table.column_names):
                line[name] = cell
            res.append(line)
    return res, request.share_url
