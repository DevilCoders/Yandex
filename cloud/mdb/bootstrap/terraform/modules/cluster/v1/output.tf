output "fqdns" {
  value = [for n in local.nodes : n.fqdn]
}

output "resources" {
  value = var.resources
}

output "service_account_id" {
  value = ycp_iam_service_account.service_account.id
}

output "backups_bucket" {
  value = var.backups_bucket ? ycp_storage_bucket.backups[0].bucket : ""
}

