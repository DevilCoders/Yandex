let t30 = {cluster='<< metadb.get("cluster") >>', service='<< metadb.get("service") >>', project='<< project_id >>', host='by_node', node='primary', dc='by_node', shard='*', name='dbaas_metadb_task_timings_<< db >>_cluster_create/30min'};

let t45 = {cluster='<< metadb.get("cluster") >>', service='<< metadb.get("service") >>', project='<< project_id >>', host='by_node', node='primary', dc='by_node', shard='*', name='dbaas_metadb_task_timings_<< db >>_cluster_create/45min'};

let t60 = {cluster='<< metadb.get("cluster") >>', service='<< metadb.get("service") >>', project='<< project_id >>', host='by_node', node='primary', dc='by_node', shard='*', name='dbaas_metadb_task_timings_<< db >>_cluster_create/60min'};

let total = avg(group_lines('sum', {cluster='<< metadb.get("cluster") >>', service='<< metadb.get("service") >>', project='<< project_id >>', host='by_node', node='primary', dc='by_node', shard='*', name='dbaas_metadb_task_timings_<< db >>_cluster_create/*'}));

let t60_c = (count(t60) > 0) ? max(t60) : 0;
let t45_c = (count(t45) > 0) ? max(t45)-t60_c : 0;
let t30_c = (count(t30) > 0) ? max(t30)-t45_c-t60_c : 0;

let over45 = t45_c + t60_c;
let is_yellow = (t30_c + t60_c + t45_c) / total >= 0.1;
let is_red = (t60_c + t45_c) / total >= 0.8 ;
let trafficColor = is_red ? 'red' : (is_yellow ? 'yellow' : 'green');

alarm_if(is_red);
warn_if(is_yellow);
