output "kms_control_plane_instances" {
  value = "${module.kms-control-plane-instance-group.instances_fqdn_ipv6}"
}

output "kms_control_plane_instance_addresses" {
  value = "${module.kms-control-plane-instance-group.all_instance_ipv6_addresses}"
}
