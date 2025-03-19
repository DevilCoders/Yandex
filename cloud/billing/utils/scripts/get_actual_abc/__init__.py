def get_actual_abc(path, yql_client):
    query = """
    $date_format = DateTime::Format("%Y-%m-%d");
$today = $date_format(Unwrap(RemoveTimezone(CurrentTzDate("Europe/Moscow"))));
SELECT
    `attributes`,
    `billing_in`,
    `bu`,
    `date`,
    `full_name`,
    `id`,
    `name`,
    `oebs_path`,
    `oebs_service`,
    `path`,
    `slug`,
    `state`,
    `tags`,
    `top_to_oebs`,
    `vs`
FROM hahn.`{path}`
where `date` = $today
    """.format(path=path)
    request = yql_client.query(query, syntax_version=1)
    request.run()
    result = request.get_results()
    if not result.is_success:
        raise RuntimeError(
            "YQL Query is not succeeded. Operation id: {}. Errors: \n{}".format(str(request.operation_id), "\n".join(
                issue.format_issue() for issue in result.errors)))
    res = []
    for table in result:
        table.fetch_full_data()
        for row in table.rows:
            line = {}
            for cell, name in zip(row, table.column_names):
                line[name] = cell
            res.append(line)
    return res, request.share_url
