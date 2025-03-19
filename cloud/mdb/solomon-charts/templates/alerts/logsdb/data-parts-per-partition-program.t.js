{% set common_selectors = "project='{project_id}', service='{service}', cluster='mdb_{cluster_id}', node='by_host'".format(project_id=project_id, service=clickhouse.service, cluster_id=logsdb.cluster_id) %}
let max_data_parts_per_partition = series_max({<< common_selectors >>, name='ch_system_async_metrics_MaxPartCountForPartition'});
let limit                        = series_min({<< common_selectors >>, name='ch_config_merge_tree_parts_to_throw_insert'});

no_data_if(count(max_data_parts_per_partition) == 0);
no_data_if(count(limit) == 0);

let max_parts_value = avg(max_data_parts_per_partition);
let limit_value = last(limit);

let result = max_parts_value;
let result_str = to_fixed(max_parts_value, 0) + ' (limit: ' + to_fixed(limit_value, 0) + ')';

let reason = 'Too many parts per partition: ' + result_str;
alarm_if(max_parts_value > 0.9 * limit_value);
warn_if(max_parts_value > 0.7 * limit_value);

let reason = 'Max parts per partition: ' + result_str;
