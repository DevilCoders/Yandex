/*
 * TODO: Find more robust way (and if possible builtin for terraform)
 *       for zookeeper config creation
 */
data "external" "zoo_cfg_getter" {
  program = [
    "python3",
    "${path.module}/files/zk_config_generator.py"
  ]

  query = {
    instance_number = 3
    template_path   = "${path.module}/files/zoo.tpl"
    environment     = var.environment
  }
}
