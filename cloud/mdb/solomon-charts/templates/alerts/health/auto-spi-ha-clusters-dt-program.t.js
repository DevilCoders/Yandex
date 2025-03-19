let total = series_sum({cluster='<< health.get('cluster') >>', service='<< health.get('service') >>', name='mdb_health_sla_clusters_<< db >>_rw_*_dbtotal_ammv', node='master', dc='by_node'}) - series_sum({cluster='<< health.get('cluster') >>', service='<< health.get('service') >>', name='mdb_health_sla_clusters_<< db >>_rw_*_dbbroken_ammv', node='master', dc='by_node'});
let dbread = series_sum({cluster='<< health.get('cluster') >>', service='<< health.get('service') >>', name='mdb_health_sla_clusters_<< db >>_rw_*_dbread_ammv', node='master', dc='by_node'});

no_data_if(count(total) == 0);
no_data_if(count(dbread) == 0);

let result = percentile(90, total - dbread);
let reason = 'DT - ' + to_fixed(result, 0);
alarm_if(result > 4);
