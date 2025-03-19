def rank_marketplace_products(src_metrics_dir, dst_table, yql_client, days_limit=0):

    query = """
    $src_metrics_dir = "{src_metrics_dir}";
    $dst_table = "{dst_table}";

    $table_paths = (
        SELECT AGGREGATE_LIST(Path) FROM (
            SELECT Path
            FROM FOLDER($src_metrics_dir, "row_count")
            WHERE Type = "table"
            AND Yson::LookupInt64(Attributes, "row_count") > 0
            ORDER BY Path DESC
            {days_limit}
        )
    );
    INSERT INTO $dst_table WITH TRUNCATE
    SELECT RANK(cnt) OVER w AS `rank`, cnt AS `count`, product_id
    FROM (
        SELECT COUNT(DISTINCT resource_id) AS cnt, product_id FROM (
            SELECT Yson::ConvertToStringList(Yson::Lookup(tags, "product_ids")) AS product_ids, resource_id
            FROM EACH($table_paths)
            WHERE `schema` = "compute.vm.generic.v1" AND Yson::Contains(tags, "product_ids")
        )
        FLATTEN LIST BY product_ids AS product_id
        GROUP BY product_id
    )
    WINDOW w AS (ORDER BY cnt DESC)
    """

    query = query.format(
        src_metrics_dir=src_metrics_dir,
        dst_table=dst_table,
        days_limit="LIMIT {}".format(days_limit) if days_limit > 0 else ""
    )

    request = yql_client.query(query, syntax_version=1)
    request.run()
    result = request.get_results()
    if not result.is_success:
        raise RuntimeError(
            "YQL Query is not succeeded. Operation id: {}. Errors: \n{}".format(str(request.operation_id), "\n".join(
                issue.format_issue() for issue in result.errors)))
