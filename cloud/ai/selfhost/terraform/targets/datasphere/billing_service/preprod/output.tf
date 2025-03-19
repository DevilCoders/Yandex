output "instance_group" {
  value = module.node_deployer_preprod.instance_group
  sensitive = true
}
