output "kms_data_plane_instances" {
  value = "${module.kms-data-plane-instance-group.instances_fqdn_ipv6}"
}

output "kms_data_plane_instance_addresses" {
  value = "${module.kms-data-plane-instance-group.all_instance_ipv6_addresses}"
}
