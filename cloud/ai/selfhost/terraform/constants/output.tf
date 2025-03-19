output "by_environment" {
  value = lookup(local.config, var.environment, null)
}
