output "ydb_dumper_instances" {
  value = "${module.kms-ydb-dumper-instance-group.instances_fqdn_ipv6}"
}

output "ydb_dumper_instance_addresses" {
  value = "${module.kms-ydb-dumper-instance-group.all_instance_ipv6_addresses}"
}
