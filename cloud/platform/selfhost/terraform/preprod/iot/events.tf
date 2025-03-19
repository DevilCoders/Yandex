locals {
  events_log_dir = "/var/log/yc-iot/events"
  events_accesslog_path = "${local.events_log_dir}/access.log"
  events_logging_secret = "/etc/yandex/statbox-push-client/logging_secret"
  events_name_prefix = "events"
}

data "template_file" "events_instance_zone" {
  count    = var.instance_count
  template = "$${zone}"
  vars = {
    zone = var.zone_suffix[element(var.zones, count.index % length(var.zones))]
  }
}

data "template_file" "events_instance_name" {
  count    = var.instance_count
  template = "$${name}"
  vars = {
    name = "${local.events_name_prefix}-${local.environment}-${data.template_file.events_instance_zone.*.rendered[count.index]}${format("%02d", floor(1 + (count.index / length(var.zone_suffix))))}"
  }
}

data "template_file" "events_user_data" {
  template = file("${path.module}/files/user-data.tpl.yaml")
  count = var.instance_count

  vars = {
    disk_id    = yandex_compute_disk.events_disk[count.index].id
  }
}

data "template_file" "events_yaml" {
  template = file("${path.module}/files/events/events.tmpl.yaml")
  count    = var.instance_count
  
  vars = {
    tvm_token = module.yav-secret-events-tvm_token.value
    log_dir = local.events_log_dir
    zone                    = data.template_file.events_instance_zone.*.rendered[count.index]
    instance_num            = count.index
  }
}

data "template_file" "events_push_client" {
  template = file("${path.module}/files/events/events-push-client.tpl.yaml")

  vars = {
    access_log_file = local.events_accesslog_path
    secret_file     = local.events_logging_secret
    client_id       = module.yav-secret-events-tvm_client_id.value
  }
}

data "template_file" "events_manifest" {
  template = file("${path.module}/files/events/events-pod.tpl.yaml")
  count = var.instance_count

  vars = {
    config_digest       = sha256(element(data.template_file.events-configs.*.rendered, count.index))
    infra_pod_spec      = module.events-infrastructure-pod.infra-pod-spec-no-push-client
    tvmtool_auth_token  = module.yav-secret-events-tvm_token.value
    log_dir             = local.events_log_dir
    push_client_version = var.push_client_version
  }
}

data "template_file" "events_tvmtool_conf" {
  template = file("${path.module}/files/events/tvmtool.tpl.conf")

  vars = {
    secret    = module.yav-secret-events-tvm_secret.value
    client_id = module.yav-secret-events-tvm_client_id.value
  }
}

data "template_file" "events-configs" {
  template = file("${path.module}/files/events/events-configs.tpl")
  count = var.instance_count

  vars = {
    events_config           = element(data.template_file.events_yaml.*.rendered, count.index)
    push_client_conf        = element(data.template_file.events_push_client.*.rendered, count.index)
    tvmtool_config          = data.template_file.events_tvmtool_conf.rendered
    logging_secret_data     = module.yav-secret-events-tvm_secret.value
  }
}

module "events-infrastructure-pod" {
  source                       = "../../modules/infrastructure-pod-metrics-agent"
  push_client_conf_path        = "${path.module}/../../common-gateway/files/push-client/push-client.yaml"
  juggler_bundle_manifest_path = "${path.module}/../../modules/infrastructure-pod-metrics-agent/files/MANIFEST.json"
  platform_http_check_path     = "${path.module}/../../modules/infrastructure-pod-metrics-agent/files/platform-http-checks.json"
  metrics_agent_conf_path      = "${path.module}/files/events/metrics-agent.yaml.tpl"
  solomon_shard_cluster        = "preprod"
}

resource "yandex_compute_disk" "events_disk" {
  count = var.instance_count
  name  = "${data.template_file.events_instance_name[count.index].rendered}-persistent"
  description = "${data.template_file.events_instance_name[count.index].rendered}-persistent"
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

module "events-instance-group" {
  source      = "../../modules/kubelet_instance_group_ytr"
  name_prefix = "${local.events_name_prefix}-${local.environment}"
  role_name   = "events"

  instance_group_size = var.instance_count

  ipv4_addrs = [
    "172.16.0.50",
    "172.17.0.45",
  ]

  ipv6_addrs = [
    "2a02:6b8:c0e:501:0:f806:0:f1",
    "2a02:6b8:c02:901:0:f806:0:28",
  ]

  cores_per_instance  = var.events_instance_cores
  memory_per_instance = var.events_instance_memory
  disk_per_instance   = var.instance_disk_size
  image_id            = var.image_id
  instance_service_account_id  = var.events_sa_id

  configs              = data.template_file.events-configs.*.rendered
  infra-configs        = module.events-infrastructure-pod.infra-pod-configs-no-push-client
  podmanifest          = data.template_file.events_manifest.*.rendered
  docker-config        = data.template_file.docker_json.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "true"

  conductor_group = "events"

  metadata = {
    osquery_tag   = "ycloud-svc-iot"
  }

  secondary_disks = yandex_compute_disk.events_disk.*.id
  metadata_per_instance = [
  for cc in data.template_file.events_user_data.*.rendered:
  map("user-data", cc)
  ]

  labels = {}

  subnets = var.subnets
}

