output "conductor_group_query" {
  value = format(
    "cloud_%v_%v_%v",
    ycp_microcosm_instance_group_instance_group.query_ig2.instance_template[0].labels.environment,
    ycp_microcosm_instance_group_instance_group.query_ig2.instance_template[0].labels.conductor-group,
    ycp_microcosm_instance_group_instance_group.query_ig2.instance_template[0].labels.conductor-role
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

output "conductor_group_lb_reader_global" {
  value = format(
    "cloud_%v_%v_%v",
    ycp_microcosm_instance_group_instance_group.reader_global_ig2.instance_template[0].labels.environment,
    ycp_microcosm_instance_group_instance_group.reader_global_ig2.instance_template[0].labels.conductor-group,
    ycp_microcosm_instance_group_instance_group.reader_global_ig2.instance_template[0].labels.conductor-role
  )
}
