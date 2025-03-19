provider "yandex" {
  endpoint  = var.yc_endpoint
  token     = var.yc_token
  folder_id = var.yc_folder
}

module "ssh-keys" {
  source       = "../../modules/ssh-keys"
  yandex_token = var.yandex_token

  // FIXME(phmx): should be ycl7 when logs are exported
  abc_service        = "cloud-platform"
  abc_service_scopes = ["administration"]
}

module "infrastructure-pod" {
  source                  = "../../modules/infrastructure-pod-metrics-agent"
  juggler_check_name      = "envoy-ping"
  solomon_shard_cluster   = "cloud_preprod_api-router"
}

data "template_file" "api-router-name" {
  count    = var.number_of_instances
  template = "$${name}"

  vars = {
    name = "api-router-preprod-${lookup(var.zone_suffix, element(var.zones, count.index))}${format("%02d", (count.index % (var.number_of_instances / length(var.zone_suffix))) + 1)}"
  }
}

data "template_file" "envoy_conf" {
  count    = var.number_of_instances
  template = file("${path.module}/files/envoy.tpl.yaml")

  vars = {
    id     = element(data.template_file.api-router-name.*.rendered, count.index)
    region = var.zone_regions[element(var.zones, count.index)]
    zone   = element(var.zones, count.index)
  }
}

data "template_file" "sds_conf" {
  count    = var.number_of_instances
  template = file("${path.module}/files/sds.tpl.yaml")

  vars = {
    id = element(data.template_file.api-router-name.*.rendered, count.index)
  }
}

data "template_file" "configs" {
  count    = var.number_of_instances
  template = file("${path.module}/files/configs.tpl")

  vars = {
    als_config               = file("${path.module}/files/als.yaml")
    yandex_internal_root_ca  = file("${path.module}/../../common/allCAs.pem")
    envoy_config             = element(data.template_file.envoy_conf.*.rendered, count.index)
    envoy_cert               = local.envoy_cert
    envoy_key                = local.envoy_key
    envoy_xds_client_cert    = local.xds_client_cert
    envoy_xds_client_key     = local.xds_client_key
    sds_config               = element(data.template_file.sds_conf.*.rendered, count.index)
    envoyconfigdumper_config = file("${path.module}/files/envoyconfigdumper.yaml")
  }
}

data "template_file" "docker_json" {
  template = file("${path.module}/files/docker-json.tpl")

  vars = {
    docker_auth = local.docker_auth
  }
}

data "template_file" "pod_manifest" {
  count    = var.number_of_instances
  template = file("${path.module}/files/envoy-pod.tpl.yaml")

  vars = {
    config_digest  = sha256(element(data.template_file.configs.*.rendered, count.index))
    infra_pod_spec = module.infrastructure-pod.infra-pod-spec-no-fluentd
  }
}

locals {
  ipv4_addrs = slice(local.all_v4_addrs, 0, var.number_of_instances) # sliced addresses list
  ipv6_addrs = slice(local.all_v6_addrs, 0, var.number_of_instances) # sliced addresses list
}

module "api-router-instance-group" {
  source          = "../../modules/kubelet_instance_group_ytr"
  name_prefix     = "api-router-preprod"
  role_name       = "router"
  conductor_group = "api-router"

  instance_group_size = var.number_of_instances
  ipv4_addrs          = local.all_v4_addrs
  ipv6_addrs          = local.all_v6_addrs
  subnets             = local.subnets

  cores_per_instance  = var.instance_cores
  memory_per_instance = var.instance_memory
  disk_per_instance   = var.instance_disk_size
  image_id            = var.image_id

  configs              = data.template_file.configs.*.rendered
  infra-configs        = module.infrastructure-pod.infra-pod-configs-no-fluentd
  podmanifest          = data.template_file.pod_manifest.*.rendered
  docker-config        = data.template_file.docker_json.rendered
  ssh-keys             = module.ssh-keys.ssh-keys

  metadata = {
    osquery_tag = "ycloud-svc-api_router"
  }

  labels = {
    layer   = "paas"
    abc_svc = "ycl7"
    env     = "pre-prod"
  }
}

