locals {
  devices_name_prefix = "devices"
}

variable "kikimr_tablespace" {
  default = "/global/iotdevices"
}

variable "devices_log_path" {
  default = "/var/log/yc-iot/devices"
}

data "template_file" "devices_push_client" {
  template = file("${path.module}/files/devices/devices-push-client.tpl.yaml")

  vars = {
    logs_path = var.devices_log_path
  }
}

data "template_file" "devices_instance_zone" {
  count    = var.instance_count
  template = "$${zone}"
  vars = {
    zone = var.zone_suffix[element(var.zones, count.index % length(var.zones))]
  }
}

data "template_file" "devices_instance_name" {
  count    = var.instance_count
  template = "$${name}"
  vars = {
    name = "${local.devices_name_prefix}-${local.environment}-${data.template_file.devices_instance_zone[count.index].rendered}${format("%02d", floor(1 + (count.index / length(var.zone_suffix))))}"
  }
}

data "template_file" "devices_user_data" {
  template = file("${path.module}/files/user-data.tpl.yaml")
  count = var.instance_count

  vars = {
    disk_id    = yandex_compute_disk.devices_disk[count.index].id
  }
}

data "template_file" "devices-configs" {
  template = file("${path.module}/files/devices/devices-configs.tpl")
  count = var.instance_count

  vars = {
    devices_cert           = local.devices_cert
    push_client_conf       = element(data.template_file.devices_push_client.*.rendered, count.index)
    log4j_conf             = element(data.template_file.devices_log4jconf.*.rendered, count.index)
  }
}

data "template_file" "devices_manifest" {
  template = file("${path.module}/files/devices/devices-pod.tpl.yaml")
  count = var.instance_count

  vars = {
    config_digest  = sha256(element(data.template_file.devices-configs.*.rendered, count.index))
    infra_pod_spec      = module.devices-infrastructure-pod.infra-pod-spec-no-push-client
    log_path       = var.devices_log_path
    push_client_version = var.push_client_version
    iot_folder          = var.yc_folder
  }
}

data "template_file" "devices_log4jconf" {
  template = file("${path.module}/files/devices/log4j2.tpl.yaml")
  count = var.instance_count

  vars = {
    log_path            = var.devices_log_path
  }
}

module "devices-infrastructure-pod" {
  source                       = "../../modules/infrastructure-pod-metrics-agent"
  push_client_conf_path        = "${path.module}/../../common-gateway/files/push-client/push-client.yaml"
  juggler_bundle_manifest_path = "${path.module}/../../modules/infrastructure-pod-metrics-agent/files/MANIFEST.json"
  platform_http_check_path     = "${path.module}/../../modules/infrastructure-pod-metrics-agent/files/platform-http-checks.json"
  metrics_agent_conf_path      = "${path.module}/files/devices/metrics-agent.yaml.tpl"
  solomon_shard_cluster        = "prod"
}

resource "yandex_compute_disk" "devices_disk" {
  count = var.instance_count
  name  = "${data.template_file.devices_instance_name[count.index].rendered}-persistent"
  description = "${data.template_file.devices_instance_name[count.index].rendered}-persistent"
  type  = "network-hdd"
  zone = element(var.zones, count.index % length(var.zones))
  size = var.data_disk_size
  labels = {
    layer = "paas"
    abc_svc = "ycmqtt"
    env = "pre-prod"
    environment = local.environment
  }
}

module "devices-instance-group" {
  source      = "../../modules/kubelet_instance_group_ytr"
  name_prefix = "${local.devices_name_prefix}-${local.environment}"
  role_name   = "devices"

  instance_group_size = var.instance_count
  instance_service_account_id = "ajep0bbterr17gdrvrk4"

  ipv4_addrs = [
    "172.16.0.34",
    "172.17.0.48",
    "172.18.0.19",
  ]

  ipv6_addrs = [
    "2a02:6b8:c0e:500:0:f803:0:3d0",
    "2a02:6b8:c02:900:0:f803:0:c3",
    "2a02:6b8:c03:500:0:f803:0:10f",
  ]

  cores_per_instance  = var.devices_instance_cores
  memory_per_instance = var.devices_instance_memory
  disk_per_instance   = var.instance_disk_size
  image_id            = var.image_id

  configs              = data.template_file.devices-configs.*.rendered
  infra-configs        = module.devices-infrastructure-pod.infra-pod-configs-no-push-client
  podmanifest          = data.template_file.devices_manifest.*.rendered
  docker-config        = data.template_file.docker_json.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "true"

  conductor_group = "devices"

  metadata = {
    osquery_tag   = "ycloud-svc-iot"
  }

  secondary_disks = yandex_compute_disk.devices_disk.*.id
  metadata_per_instance = [
    for cc in data.template_file.devices_user_data.*.rendered:
    map("user-data", cc)
  ]

  labels = {}

  subnets = var.subnets
}
