{% set common_selectors  = "project='{project_id}', service='{service}', cluster='mdb_{cluster_id}', node='by_host'".format(project_id=project_id, service=clickhouse.service, cluster_id=logsdb.cluster_id) %}
let failed_queries = series_max({<< common_selectors >>, name='ch_system_events_FailedInsertQuery_rate'});
let total_queries  = series_max({<< common_selectors >>, name='ch_system_events_InsertQuery_rate'});

no_data_if(count(failed_queries) == 0);
no_data_if(count(total_queries) == 0);

let result = 100 * sum(failed_queries) / sum(total_queries);
let result_str = to_fixed(result, 2) + '%';

let reason = 'Failed queries: ' + result_str;
alarm_if(result > 20);
warn_if(result > 10);
