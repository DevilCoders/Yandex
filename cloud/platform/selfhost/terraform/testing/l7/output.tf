output "cpl" {
  value = <<EOT
    IG: ${module.cpl.instance_group_id}
    TG: ${module.cpl.target_group_id}
  EOT
}

output "api-router" {
  value = <<EOT
    IG: ${module.api.instance_group_id}
    TG: ${module.api.target_group_id}
  EOT
}
