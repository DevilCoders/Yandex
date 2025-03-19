output "trail_alb_target_group_id" {
  value = "${ycp_platform_alb_target_group.trail_target_group.id}"
}

output "trail_alb_backend_group_id" {
  value = "${ycp_platform_alb_backend_group.trail_backend_group.id}"
}
