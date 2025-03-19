data "template_file" "envoy_config" {
    template = file("${path.module}/files/envoy.tpl.yml")
    vars = {
        envoy_node_proxy_xds_address = var.envoy_node_proxy_xds_address
    }
}

output "rendered" {
    value = data.template_file.envoy_config.rendered
}
