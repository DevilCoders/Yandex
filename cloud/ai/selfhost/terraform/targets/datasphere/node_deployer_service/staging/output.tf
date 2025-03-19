output "instance_group" {
  value = module.node_deployer_staging.instance_group
  sensitive = true
}
