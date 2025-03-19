output "application_version" {
  value = var.application_version
}

output "tool_version" {
  value = var.tool_version
}

output "ydb-dumper_version" {
  value = var.ydb-dumper_version
}

output "image_id" {
  value = var.image_id
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

output "solomon_agent_image_version" {
  value = var.solomon_agent_image_version
}

output "folder_id" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-certificate-manager-folder-id
}

output "service_account_id" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-certificate-manager-service-account-id
}

output "subnets" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-certificate-manager-subnets
}

output "dns_zone_id" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-certificate-manager-dns-zone-id
}

output "cloud-init-sh" {
  value = data.template_file.cloud-init-sh.rendered
}

output "cpl-placement-group-id" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-certificate-manager-cpl-pg-id
}

output "dpl-placement-group-id" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-certificate-manager-dpl-pg-id
}

output "validation-placement-group-id" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-certificate-manager-validation-pg-id
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
