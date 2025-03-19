output "instance_group" {
  value = module.ds_billing_service_staging.instance_group
  sensitive = true
}
