output "trail_exporter_instances" {
  value = "${module.exporter-instance-group.instances_fqdn_ipv6}"
}

output "trail_exporter_instance_addresses" {
  value = "${module.exporter-instance-group.all_instance_ipv6_addresses}"
}
