variable "upstream_port" {
  default = 8088
}

variable "downstream_port" {
  default = 17004
}

data "template_file" "envoy_config" {
  template = file("${path.module}/files/envoy.tpl.yml")

  vars = {
    upstream_port = var.upstream_port
    downstream_port = var.downstream_port
  }
}

output "rendered" {
  value = data.template_file.envoy_config.rendered
}
