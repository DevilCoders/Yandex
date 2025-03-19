output group {
  value = ycp_microcosm_instance_group_instance_group.group
}

output group_data {
  value = data.yandex_compute_instance_group.group_data
}

output dns_suffix {
  value = local.dns_suffix
}

output "conductor_group" {
  value = format(
  "cloud_%v_%v_%v",
  ycp_microcosm_instance_group_instance_group.group.instance_template[0].labels.environment,
  ycp_microcosm_instance_group_instance_group.group.instance_template[0].labels.conductor-group,
  ycp_microcosm_instance_group_instance_group.group.instance_template[0].labels.conductor-role
  )
}

output l7_target_group {
  value = ycp_microcosm_instance_group_instance_group.group.platform_l7_load_balancer_state != null ? ycp_microcosm_instance_group_instance_group.group.platform_l7_load_balancer_state[0].target_group_id : ""
}