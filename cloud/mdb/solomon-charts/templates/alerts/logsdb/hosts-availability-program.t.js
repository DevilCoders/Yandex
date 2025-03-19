{% set common_selectors     = "project='{project_id}', service='{service}', cluster='mdb_{cluster_id}', shard!='none', node='by_host'".format(project_id=project_id, service=clickhouse.service, cluster_id=logsdb.cluster_id) %}
{% set can_read_timeseries  = "replace_nan({{{common_selectors}, name='can_read'}}, 0)".format(common_selectors=common_selectors) %}
{% set can_write_timeseries = "replace_nan({{{common_selectors}, name='can_write'}}, 0)".format(common_selectors=common_selectors) %}
let hosts            = series_max({<< common_selectors >>, name='clickhouse_host_count'});
let can_read_hosts   = series_sum(<< can_read_timeseries >>);
let can_write_hosts  = series_sum(<< can_write_timeseries >>);
let healthy_hosts    = series_sum(<< can_read_timeseries >> * << can_write_timeseries >>);
let shards           = series_max({<< common_selectors >>, name='clickhouse_shard_count'});
let available_shards = series_sum((max(<< can_read_timeseries >>) by shard) * (max(<< can_write_timeseries >>) by shard));

let reason = 'No healthy hosts';
alarm_if(count(hosts) == 0 || count(healthy_hosts) == 0 || max(healthy_hosts) == 0);

let unhealthy_shard_count = max(shards) - max(available_shards);
let reason = to_fixed(unhealthy_shard_count, 0) + ' shard(s) unavailable';
alarm_if(unhealthy_shard_count > 0);

let unhealthy_host_count = max(hosts) - max(healthy_hosts);
let reason = to_fixed(unhealthy_host_count, 0) + ' host(s) unhealthy';
warn_if(unhealthy_host_count > 0);

let reason = 'OK';
