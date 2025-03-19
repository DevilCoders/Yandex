output "application_version" {
  value = var.application_version
}

output "ydb-dumper_version" {
  value = var.ydb-dumper_version
}

output "overlay_image_id" {
  value = var.overlay_image_id
}

output "metadata_image_version" {
  value = var.metadata_image_version
}

output "config_server_image_version" {
  value = var.config_server_image_version
}

output "api_gateway_image_version" {
  value = var.api_gateway_image_version
}

output "envoy_image_version" {
  value = var.envoy_image_version
}

output "folder_id" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-lockbox-folder-id
}

output "service_account_id" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-lockbox-service-account-id
}

output "ydb_service_account_id" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-lockbox-ydb-service-account-id
}

output "subnets" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-lockbox-subnets
}

output "dns_zone_id" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-lockbox-dns-zone-id
}

output "cloud-init-sh" {
  value = data.template_file.cloud-init-sh.rendered
}

output "yc_zones" {
  value = var.yc_zones
}

output "hostname_suffix" {
  value = var.hostname_suffix
}

output "instance_platform_id" {
  value = var.instance_platform_id
}

output "ycp_profile" {
  value = var.ycp_profile
}

output "yc_zone" {
  value = var.yc_zone
}

output "yc_region" {
  value = var.yc_region
}

output "abc_group" {
  value = var.abc_group
}

output "instance_labels" {
  value = var.instance_labels
}

output "osquery_tag" {
  value = var.osquery_tag
}

output "yc_zone_suffix" {
  value = var.yc_zone_suffix
}
