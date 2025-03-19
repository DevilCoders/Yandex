output "user_nets" {
  value = local.user_nets[var.environment]
}

output "control_nets_yandex_ru" {
  value = local.control_nets_yandex_ru[var.environment]
}

output "control_nets_ds_ipv6" {
  value = local.control_nets_ds_ipv6[var.environment]
}

output "cloud_ml_dev_nets" {
  value = local.cloud_ml_dev_nets[var.environment]
}
