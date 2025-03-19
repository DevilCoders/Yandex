{% set common_selectors  = "project='{project_id}', service='dom0', cluster='internal-mdb_dom0', cid='{cid}', host='by_cid_container'".format(project_id=project_id, cid=logsdb.cluster_id) %}
let memory_utilization = series_max(100 * {<< common_selectors >>, name='/porto/anon_usage'} / {<< common_selectors >>, name='/porto/memory_guarantee'});

no_data_if(count(memory_utilization) == 0);

let result = max(memory_utilization);
let result_str = to_fixed(result, 2) + '%';

let reason = 'High memory utilization: ' + result_str;
alarm_if(result > 95);
warn_if(result > 80);

let reason = 'Memory utilization: ' + result_str;
