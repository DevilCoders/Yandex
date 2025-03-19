def run_query(query, yql_client, yt_wrapper):
    request = yql_client.query(query, syntax_version=1)
    request.run()
    result = request.get_results()
    if not result.is_success:
        raise RuntimeError(
            "YQL Query is not succeeded. URL: {}. Errors: \n{}".format(str(request.share_url), "\n".join(
                issue.format_issue() for issue in result.errors)))
    res = []
    for table in result:
        for row in table.rows:
            line = {}
            for cell, name in zip(row, table.column_names):
                line[name] = cell
            res.append(line)
    return res, request.share_url
