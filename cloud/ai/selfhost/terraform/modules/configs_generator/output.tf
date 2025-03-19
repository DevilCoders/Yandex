output "configs" {
  value = local.configs
}

output "configs_rendered" {
  value = jsonencode(local.configs)
}