let failed = {cluster='<< metadb.get("cluster") >>', service='<< metadb.get("service") >>', project='<< project_id >>', host='by_node', node='primary', dc='by_node', shard='*', name='dbaas_metadb_task_status_all/failed'};

let pending = {cluster='<< metadb.get("cluster") >>', service='<< metadb.get("service") >>', project='<< project_id >>', host='by_node', node='primary', dc='by_node', shard='*', name='dbaas_metadb_task_status_all/pending'};

let processing = {cluster='<< metadb.get("cluster") >>', service='<< metadb.get("service") >>', project='<< project_id >>', host='by_node', node='primary', dc='by_node', shard='*', name='dbaas_metadb_task_status_all/processing'};

let succeeded = {cluster='<< metadb.get("cluster") >>', service='<< metadb.get("service") >>', project='<< project_id >>', host='by_node', node='primary', dc='by_node', shard='*', name='dbaas_metadb_task_status_all/succeeded'};

let failed_c =  (count(failed) > 0) ? max(failed) : 0;
let pending_c =  (count(pending) > 0) ? max(pending) : 0;
let processing_c =  (count(processing) > 0) ? max(processing) : 0;
let succeeded_c =  (count(succeeded) > 0) ? max(succeeded) : 0;

let v = failed_c/(pending_c+processing_c+succeeded_c+1);

let is_yellow = v > 0;
let is_red = v >= 0.04;
let trafficColor = is_red ? 'red' : (is_yellow ? 'yellow' : 'green');
alarm_if(is_red);
warn_if(is_yellow);
