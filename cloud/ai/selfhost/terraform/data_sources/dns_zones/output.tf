output "ds_private_dns_zone" {
  value = local.ds_private_dns_zone[var.environment]
}

output "ds_public_dns_zone" {
  value = local.ds_public_dns_zone[var.environment]
}
