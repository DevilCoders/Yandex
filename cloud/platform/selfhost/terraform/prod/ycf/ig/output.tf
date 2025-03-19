output group {
  value = yandex_compute_instance_group.group
}

output dns_suffix {
  value = local.dns_suffix
}

output "conductor_group" {
  value = format(
  "cloud_%v_%v_%v",
yandex_compute_instance_group.group.instance_template[0].labels.environment,
yandex_compute_instance_group.group.instance_template[0].labels.conductor-group,
yandex_compute_instance_group.group.instance_template[0].labels.conductor-role
)
}
