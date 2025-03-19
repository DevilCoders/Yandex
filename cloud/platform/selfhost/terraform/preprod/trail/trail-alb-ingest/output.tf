output "trail_ingest_alb_target_group_id" {
  value = "${ycp_platform_alb_target_group.trail_ingest_target_group.id}"
}

output "trail_ingest_alb_backend_group_id" {
  value = "${ycp_platform_alb_backend_group.trail_ingest_backend_group.id}"
}

output "trail_ingest_certificate_internal_id" {
  value = "${ycp_certificatemanager_certificate_request.trail_ingest_certificate_internal.id}"
}

output "trail_ingest_vpc_address_id" {
  value = "${ycp_vpc_address.trail_ingest_vpc_address.id}"
}

output "trail_ingest_alb_load_balancer_id" {
  value = "${ycp_platform_alb_load_balancer.trail_ingest_alb_load_balancer.id}"
}

output "trail_ingest_alb_http_router_id" {
  value = "${ycp_platform_alb_http_router.trail_ingest_alb_http_router.id}"
}

output "trail_ingest_alb_virtual_host_id" {
  value = "${ycp_platform_alb_virtual_host.trail_ingest_alb_virtual_host.id}"
}
