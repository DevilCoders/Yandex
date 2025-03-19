{% set common_selectors  = "project='{project_id}', service='{service}', cluster='mdb_{cluster_id}', node='by_host'".format(project_id=project_id, service=clickhouse.service, cluster_id=logsdb.cluster_id) %}
let queries = series_max({<< common_selectors >>, name='ch_system_events_InsertQuery_rate'});

no_data_if(count(queries) == 0);

let result = max(queries);
let result_str = to_fixed(result, 2);

let reason = 'INSERT queries rate is too low: ' + result_str;
alarm_if(result < 1);

let reason = 'INSERT queries rate: ' + result_str;
