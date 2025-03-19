{% set common_selectors  = "project='{project_id}', service='{service}', cluster='mdb_{cluster_id}', node='by_host'".format(project_id=project_id, service=clickhouse.service, cluster_id=logsdb.cluster_id) %}
let avg_query_duration = series_max({<< common_selectors >>, name='ch_system_events_QueryTimeMicroseconds_rate'} / {<< common_selectors >>, name='ch_system_events_Query_rate'} / 1000000);

no_data_if(count(avg_query_duration) == 0);

let result = percentile(90, avg_query_duration);
let result_str = to_fixed(result, 1) + ' seconds';

let reason = 'Average query duration: ' + result_str;
alarm_if(result > 5);
warn_if(result > 2);
