output "application_version" {
  value = var.application_version
}

output "tool_version" {
  value = var.tool_version
}

output "monitoring_version" {
  value = var.monitoring_version
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

output "solomon_agent_image_version" {
  value = var.solomon_agent_image_version
}

output "folder_id" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-kms-folder-id
}

output "service_account_id" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-kms-service-account-id
}

output "ydb_service_account_id" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-kms-ydb-service-account-id
}

output "subnets" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-kms-subnets
}

output "dns_zone_id" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-kms-dns-zone-id
}

output "cpl-placement-group-id" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-kms-cpl-pg-id
}

output "dpl-placement-group-id" {
  value = data.terraform_remote_state.bootstrap-resources.outputs.yc-kms-dpl-pg-id
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
