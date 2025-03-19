output "lockbox_ydb-dumper_instances" {
  value = "${module.lockbox-ydb-dumper-instance-group.instances_fqdn_ipv6}"
}

output "lockbox_data_plane_instance_addresses" {
  value = "${module.lockbox-ydb-dumper-instance-group.all_instance_ipv6_addresses}"
}
