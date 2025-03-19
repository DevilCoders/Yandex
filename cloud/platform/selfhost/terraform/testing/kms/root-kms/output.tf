output "root_kms_instances" {
  value = "${module.root-kms-instance-group.instances_fqdn_ipv6}"
}

output "root_kms_instance_addresses" {
  value = "${module.root-kms-instance-group.all_instance_ipv6_addresses}"
}
