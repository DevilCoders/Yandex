output "network_id" {
  value = ycp_vpc_network.default.id
}

output "subnet_ids" {
  value = {
    for subnet in ycp_vpc_subnet.default:
    subnet.zone_id => subnet.id
  }
}
