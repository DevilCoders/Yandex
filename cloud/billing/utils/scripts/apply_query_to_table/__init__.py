def apply_query_to_table(yql_client, yt_wrapper, query, outputs=None, outputs_primary_keys=None):
    result_query = """

        {query}

        {outputs}
    """
    outputs_primary_keys = outputs_primary_keys or {}

    output_queries = []
    if outputs and ":" in outputs:   # at least one pair variable:table
        for output in outputs.split(','):
            variable, path = output.split(':')
            path = '`' + path + '`' if '/' in path else '$' + path
            output_query = 'INSERT INTO {} WITH TRUNCATE SELECT * FROM ${}'.format(path, variable)

            if variable in outputs_primary_keys:
                output_query += ' ORDER BY {}'.format(outputs_primary_keys[variable])

            output_query += ';'
            output_queries.append(output_query)

    result_query = result_query.format(query=query, outputs=''.join(output_queries))

    request = yql_client.query(result_query, syntax_version=1)
    request.run()
    result = request.get_results()
    if not result.is_success:
        raise RuntimeError(
            'YQL Query is not succeeded. Operation id: {}. Errors: \n{}'.format(str(request.operation_id), '\n'.join(
                issue.format_issue() for issue in result.errors)))
