//noinspection HILUnresolvedReference
output "cpl" {
  value = module.cpl.group_data.instances != null ? compact(split("\n", <<EOT
%{ for instance in module.cpl.group_data.instances  ~}
${format("%v (%v): %v%v [%v]",
  instance.instance_id,
  local.zones_short[instance.zone_id],
  instance.fqdn,
  module.cpl.dns_suffix,
  instance.network_interface[0].ipv6_address
)}
%{ endfor ~}
EOT
)) : []
}

output "dpl" {
  value = module.dpl.group_data.instances != null ? compact(split("\n", <<EOT
%{ for instance in module.dpl.group_data.instances  ~}
${format("%v (%v): %v%v [%v]",
  instance.instance_id,
  local.zones_short[instance.zone_id],
  instance.fqdn,
  module.dpl.dns_suffix,
  instance.network_interface[0].ipv6_address
)}
%{ endfor ~}
EOT
)) : []
}


output "cpl_conductor_group" {
  value = module.cpl.conductor_group
}

output "cpl_backend_group" {
  value = ycp_platform_alb_backend_group.mdbproxy_cpl_backend_group.id
}

output "dpl_conductor_group" {
    value = module.dpl.conductor_group
}
