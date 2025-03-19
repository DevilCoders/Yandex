output "kms_devel_hsm_instances" {
  value = "${module.kms-devel-hsm-instance-group.instances_fqdn_ipv6}"
}

output "kms_data_plane_instance_addresses" {
  value = "${module.kms-devel-hsm-instance-group.all_instance_ipv6_addresses}"
}
