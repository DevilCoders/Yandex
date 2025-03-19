locals {
  configs = {
    for config_path, config_content in merge(var.input_configs...) :
    config_path => { content = config_content }
  }
}
