{% set common_selectors  = "project='{project_id}', service='{service}', cluster='mdb_{cluster_id}', node='by_host'".format(project_id=project_id, service=clickhouse.service, cluster_id=logsdb.cluster_id) %}
let max_replication_delay = series_max({<< common_selectors >>, name='ch_system_async_metrics_ReplicasMaxAbsoluteDelay'});

no_data_if(count(max_replication_delay) == 0);

let result = percentile(90, max_replication_delay);
let result_str = to_fixed(result, 0) + ' seconds';

let reason = 'Max replication delay across tables: ' + result_str;
alarm_if(result > 3600);
warn_if(result > 60);
