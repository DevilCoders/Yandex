let total = series_avg({cluster='yc.mdb.control-plane', project='yc.mdb.serviceCloud', name='clusters_<<cluster_type>>_rw_compute_prod_dbtotal'});
let read = series_avg({cluster='yc.mdb.control-plane', project='yc.mdb.serviceCloud', name='clusters_<<cluster_type>>_rw_compute_prod_dbread'});
let write = series_avg({cluster='yc.mdb.control-plane', project='yc.mdb.serviceCloud', name='clusters_<<cluster_type>>_rw_compute_prod_dbwrite'});
let broken = series_avg({cluster='yc.mdb.control-plane', project='yc.mdb.serviceCloud', name='clusters_<<cluster_type>>_rw_compute_prod_dbbroken'});

let expected_ok = last(total) - last(broken);

let problematic_read = expected_ok - last(read);
let problematic_write = expected_ok - last(write);

let is_red = problematic_read >= 1 || problematic_write >= 1;

let message = is_red ? 'problem_read: ' + problematic_read + ' , problem_write: ' + problematic_write : 'OK';

alarm_if(is_red);
