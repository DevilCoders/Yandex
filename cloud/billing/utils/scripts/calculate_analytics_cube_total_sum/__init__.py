def calculate_analytics_cube_total_sum(src_table_path, dst_table_path, start_date, yql_client, yt_wrapper):
    tables = """
    $src_table = "{src_table}";
    $dst_table = "{dst_table}";
    $start_date = "{start_date}";
    """
    query = """
    $format = DateTime::Format("%Y-%m-%d");
    $get_date_by_timestamp = ($timestamp) -> { RETURN $format(AddTimezone(DateTime::FromSeconds(CAST($timestamp AS Uint32)), 'Europe/Moscow')); };

    $total_with_date = (SELECT billing_record_cost, billing_record_credit, $get_date_by_timestamp(billing_record_end_time) as `date`
    FROM $src_table);

    $now = DateTime::ToSeconds(CurrentUtcTimestamp());

    $yesterday = $get_date_by_timestamp($now - 60 * 60 * 24);

    $before_yesterday = $get_date_by_timestamp($now - 2 * 60 * 60 * 24);

    $total_sums_by_date = (SELECT SUM(billing_record_cost) as cost, SUM(billing_record_credit) as credit, `date` from $total_with_date
    WHERE `date` >= $start_date
    GROUP BY `date`);

    $yesterday_cost = (SELECT SUM(cost) as cost FROM $total_sums_by_date WHERE `date` <= $yesterday);
    $yesterday_credit = (SELECT SUM(credit) as credit FROM $total_sums_by_date WHERE `date` <= $yesterday);

    $before_yesterday_cost = (SELECT SUM(cost) as cost FROM $total_sums_by_date WHERE `date` <= $before_yesterday);
    $before_yesterday_credit = (SELECT SUM(credit) as credit FROM $total_sums_by_date WHERE `date` <= $before_yesterday);

    $result = (
    SELECT CAST($before_yesterday as STRING) as `date`, $before_yesterday_cost as cost, $before_yesterday_credit as credit
    UNION ALL
    SELECT CAST($yesterday AS STRING) AS `date`, $yesterday_cost as cost, $yesterday_credit as credit
    );

    INSERT INTO $dst_table WITH TRUNCATE SELECT * FROM $result;
    """

    tables = tables.format(src_table=src_table_path, dst_table=dst_table_path, start_date=start_date or "1970-01-01")

    request = yql_client.query(tables + query, syntax_version=1)
    request.run()
    result = request.get_results()
    if not result.is_success:
        raise RuntimeError(
            "YQL Query is not succeeded. Operation id: {}. Errors: \n{}".format(str(request.operation_id), "\n".join(
                issue.format_issue() for issue in result.errors)))
