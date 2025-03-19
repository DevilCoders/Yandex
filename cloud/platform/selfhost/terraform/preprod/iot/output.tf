output "mqtt__instances" {
  value = formatlist("%v", yandex_compute_instance.mqtt-instance-group.*.name)
}

output "devices__instances" {
  value = module.devices-instance-group.instances_fqdn_ipv6
}

output "devices__instance_addresses" {
  value = module.devices-instance-group.all_instance_ipv6_addresses
}

output "devices__instance_addresses_ipv4" {
  value = module.devices-instance-group.all_instance_ipv4_addresses
}

output "devices__instances_full_info" {
  value = formatlist(
    "instance [%v] has ipv6 %v and name %v",
    module.devices-instance-group.all_instance_ids,
    module.devices-instance-group.all_instance_ipv6_addresses,
    module.devices-instance-group.all_instance_group_names,
  )
}

output "events__instances" {
  value = module.events-instance-group.instances_fqdn_ipv6
}

output "events__instance_addresses" {
  value = module.events-instance-group.all_instance_ipv6_addresses
}

output "events__instance_addresses_ipv4" {
  value = module.events-instance-group.all_instance_ipv4_addresses
}

output "events__instances_full_info" {
  value = formatlist(
  "instance [%v] has ipv6 %v and name %v",
  module.events-instance-group.all_instance_ids,
  module.events-instance-group.all_instance_ipv6_addresses,
  module.events-instance-group.all_instance_group_names,
  )
}
