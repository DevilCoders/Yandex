output "conductor_group_query_ydb" {
  value = format(
    "cloud_%v_%v_%v",
    ycp_microcosm_instance_group_instance_group.query_ydb2_ig.instance_template[0].labels.environment,
    ycp_microcosm_instance_group_instance_group.query_ydb2_ig.instance_template[0].labels.conductor-group,
    ycp_microcosm_instance_group_instance_group.query_ydb2_ig.instance_template[0].labels.conductor-role
  )
}

output "conductor_group_collector" {
  value = format(
    "cloud_%v_%v_%v",
    ycp_microcosm_instance_group_instance_group.collector_lb_ig.instance_template[0].labels.environment,
    ycp_microcosm_instance_group_instance_group.collector_lb_ig.instance_template[0].labels.conductor-group,
    ycp_microcosm_instance_group_instance_group.collector_lb_ig.instance_template[0].labels.conductor-role
  )
}

output "conductor_group_lb_reader_ydb" {
  value = format(
    "cloud_%v_%v_%v",
    ycp_microcosm_instance_group_instance_group.reader_ydb2_ig.instance_template[0].labels.environment,
    ycp_microcosm_instance_group_instance_group.reader_ydb2_ig.instance_template[0].labels.conductor-group,
    ycp_microcosm_instance_group_instance_group.reader_ydb2_ig.instance_template[0].labels.conductor-role
  )
}
