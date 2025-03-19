{% set common_selectors  = "project='{project_id}', service='{service}', cluster='mdb_{cluster_id}', node='by_host'".format(project_id=project_id, service=clickhouse.service, cluster_id=logsdb.cluster_id) %}
let disk_space_utilization = series_max(100 * {<< common_selectors >>, name='disk-used_bytes_/var/lib/clickhouse'} / {<< common_selectors >>, name='disk-total_bytes_/var/lib/clickhouse'});

no_data_if(count(disk_space_utilization) == 0);

let result = max(disk_space_utilization);
let result_str = to_fixed(result, 2) + '%';

let reason = 'Disk space utilization: ' + result_str;
alarm_if(result > 95);
warn_if(result > 90);
