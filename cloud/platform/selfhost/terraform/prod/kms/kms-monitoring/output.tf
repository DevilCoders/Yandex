output "kms_monitoring_instances" {
  value = "${module.kms-monitoring-instance-group.instances_fqdn_ipv6}"
}

output "kms_monitoring_instance_addresses" {
  value = "${module.kms-monitoring-instance-group.all_instance_ipv6_addresses}"
}
