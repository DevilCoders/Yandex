output "placement_group_id" {
  value = ycp_compute_placement_group.dns_proxy.id
}

output "image_id" {
  value = ycp_compute_image.dns_proxy.id
}
