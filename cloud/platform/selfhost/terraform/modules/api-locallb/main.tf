data "template_file" "envoy_config" {
  template = file("${path.module}/files/envoy.tpl.yaml")

  vars = {
    max_heap_size_bytes = var.envoy_heap_memory
    api_port            = var.api_port
    hc_port             = var.hc_port
    private_hc_port     = var.private_hc_port
  }
}

data "template_file" "private_envoy_config" {
  template = file("${path.module}/files/private-envoy.tpl.yaml")

  vars = {
    max_heap_size_bytes         = var.envoy_heap_memory
    api_port                    = var.private_api_port
    hc_port                     = var.private_hc_port
    request_timeout             = var.request_timeout
    num_retries                 = var.num_retries
    per_try_timeout             = var.per_try_timeout
    lb_endpoints                = var.envoy_endpoints_string
    trusted_ca                  = var.trusted_ca
    endpoint_certificate_suffix = var.private_endpoint
    private_api_cert_crt        = var.private_api_cert_crt
    private_api_cert_key        = var.private_api_cert_key
  }
}

locals {
  zones = [
  for i, zone in var.zones :
  "{ location: ${zone} }"
  ]
  zones_string = "[${join(", ", local.zones)}]"
}

data "template_file" "gateway_services_config" {
  template = file("${path.module}/files/gateway-services.tpl.yaml")

  vars = {
    region               = var.region
    zones                = local.zones_string
    endpoint             = var.endpoint
    api_port             = var.api_port
    operation_service_id = var.operation_service_id
    service_id           = var.service_id
    id_prefix            = var.id_prefix
    private_api_port     = var.private_api_port
  }
}

data "template_file" "api_configs" {
  template = file("${path.module}/files/configs.tpl")

  vars = {
    envoy_config         = data.template_file.envoy_config.rendered
    private_envoy_config = data.template_file.private_envoy_config.rendered
    gateway_config       = file("${path.module}/files/gateway.yaml")
    configserver_config  = file("${path.module}/files/configserver.yaml")
    envoy_resources      = file("${path.module}/files/envoy-resources.yaml")
    gateway_services     = data.template_file.gateway_services_config.rendered
  }
}

data "template_file" "api_podmanifest" {
  template = file("${path.module}/files/podmanifest.tpl.yaml")

  vars = {
    api_config_digest       = sha256(data.template_file.api_configs.rendered)
    metadata_version        = var.metadata_version
    configserver_version    = var.configserver_version
    configserver_pod_memory = var.configserver_pod_memory
    gateway_version         = var.gateway_version
    gateway_pod_memory      = var.gateway_pod_memory
    envoy_version           = var.envoy_version
    envoy_pod_memory        = var.envoy_pod_memory
  }
}
