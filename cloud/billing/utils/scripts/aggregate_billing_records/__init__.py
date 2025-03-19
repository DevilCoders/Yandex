def aggregate_billing_records(src_dir_path, dst_table_path, yql_client, yt_wrapper, tmp_dir=None):
    query = """
        PRAGMA yt.Pool = 'cloud_analytics_pool';
        {tmp_dir_pragma}
        $src_dir = "{src_dir}";
        $dst_dir = "{dst_dir}";
        $dst_table_name = "{dst_table}";

        $dst_table = $dst_dir || "/" || $dst_table_name;

        $table_paths =
          (SELECT Path
           FROM FOLDER($src_dir, "row_count")
           WHERE Type = "table"
             AND JSON::LookupInt64(Attributes, "row_count") > 0);

        $path_list =
          (SELECT AGGREGATE_LIST(Path)
           FROM $table_paths);

        $unique_logfeller_rows =
          (SELECT billing_account_id,
                  end_time,
                  cloud_id,
                  sku_id,
                  labels_hash,
                  some(start_time) AS start_time,
                  some(pricing_quantity) AS pricing_quantity,
                  some(cost) AS cost,
                  some(credit) AS credit,
                  some(credit_charges) AS credit_charges,
                  some(created_at) AS created_at
           FROM EACH($path_list) with INFER_SCHEMA
           GROUP BY (billing_account_id,
                     end_time,
                     cloud_id,
                     sku_id,
                     labels_hash));

        $rows_to_add =
          (SELECT f.billing_account_id AS billing_account_id,
                  f.end_time AS end_time,
                  f.cloud_id AS cloud_id,
                  f.sku_id AS sku_id,
                  f.labels_hash AS labels_hash,
                  f.start_time AS start_time,
                  f.pricing_quantity AS pricing_quantity,
                  f.cost AS cost,
                  f.credit AS credit,
                  f.credit_charges AS credit_charges,
                  f.created_at AS created_at
           FROM $unique_logfeller_rows AS f
           LEFT JOIN $dst_table AS s ON f.billing_account_id == s.billing_account_id
           AND f.end_time == s.end_time
           AND f.cloud_id == s.cloud_id
           AND f.sku_id == s.sku_id
           AND f.labels_hash == s.labels_hash
           WHERE s.billing_account_id IS NULL);

        DEFINE ACTION $insert_with_join() AS
        INSERT INTO $dst_table WITH TRUNCATE
        SELECT * FROM $rows_to_add
        UNION ALL
        SELECT * FROM $dst_table;
        END DEFINE;

        DEFINE ACTION $insert_without_join() AS
        INSERT INTO $dst_table
        SELECT *
        FROM $unique_logfeller_rows;

        END DEFINE;

        $table_exists =
          (SELECT count(*)
           FROM FOLDER($dst_dir, "key")
           WHERE Type = "table"
             AND JSON::LookupString(Attributes, "key") == $dst_table_name);

        EVALUATE IF ($table_exists == 1) DO $insert_with_join() ELSE DO $insert_without_join();
    """

    tmp_dir_pragma = '''
    PRAGMA yt.TmpFolder="{tmp_dir}";
    PRAGMA yt.QueryCacheMode="disable";
    PRAGMA yt.ReleaseTempData="immediate";
    PRAGMA yt.AutoMerge="disabled";'''.format(tmp_dir=tmp_dir) if tmp_dir else ""

    slash_pos = dst_table_path.rfind("/")
    dst_dir_path = dst_table_path[:slash_pos]
    dst_table_name = dst_table_path[slash_pos + 1:]
    request = yql_client.query(
        query.format(src_dir=src_dir_path, dst_dir=dst_dir_path,
                     dst_table=dst_table_name, tmp_dir_pragma=tmp_dir_pragma),
        syntax_version=1)
    request.run()
    result = request.get_results()
    if not result.is_success:
        raise RuntimeError(
            "YQL Query is not succeeded. Operation id: {}. Errors: \n{}".format(str(request.operation_id), "\n".join(
                issue.format_issue() for issue in result.errors)))
