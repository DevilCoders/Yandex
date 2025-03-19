PRAGMA Library("schema_h_after_trial.sql");
PRAGMA Library("time.sql");

IMPORT schema_h_after_trial SYMBOLS $consumption_for_schema;

$bm_metrics_table = "//home/logfeller/logs/yc-billing-metrics/1d";
$result_table="home/cloud_analytics/tmp/antifraud/all_1h_after_trial";

insert into $result_table with truncate
select *
from $consumption_for_schema($bm_metrics_table, 'all')
