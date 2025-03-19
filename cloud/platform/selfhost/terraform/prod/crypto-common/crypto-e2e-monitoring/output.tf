output "e2e_monitoring_instances" {
  value = "${module.e2e-monitoring-instance-group.instances_fqdn_ipv6}"
}

output "e2e_monitoring_instance_addresses" {
  value = "${module.e2e-monitoring-instance-group.all_instance_ipv6_addresses}"
}
