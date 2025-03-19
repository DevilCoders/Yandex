{% set common_selectors  = "project='{project_id}', service='dom0', cluster='internal-mdb_dom0', cid='{cid}', host='by_cid_container'".format(project_id=project_id, cid=logsdb.cluster_id) %}
let cpu_utilization = series_max(100 * {<< common_selectors >>, name='/porto/cpu_usage'} / {<< common_selectors >>, name='/porto/cpu_guarantee'});

no_data_if(count(cpu_utilization) == 0);

let result = percentile(90, cpu_utilization);
let result_str = to_fixed(result, 2) + '%';

let reason = 'High CPU utilization: ' + result_str;
alarm_if(result > 90);
warn_if(result > 80);

let reason = 'CPU utilization: ' + result_str;
