locals {
  mqtt_name_prefix = "mqtt"
  environment = "preprod"
  version     = "1_0"

  zones = {
    "vla" = "ru-central1-a"
    "sas" = "ru-central1-b"
  }

  zone_key = "zone"
  ipv4_key = "ipv4"
  ipv6_key = "ipv6"

  subnets = {
    "vla" = "bucpba0hulgrkgpd58qp"
    "sas" = "bltueujt22oqg5fod2se"
    "myt" = "fo27jfhs8sfn4u51ak2s"
  }

  mqtt_insances = [
    {
      "zone" = "vla"
      "ipv4" = "172.16.0.13"
      "ipv6" = "2a02:6b8:c0e:501:0:f806:0:16f"
    },
    {
      "zone" = "sas"
      "ipv4" = "172.17.0.44"
      "ipv6" = "2a02:6b8:c02:901:0:f806:0:403"
    },
  ]
  mqtt_logs_dir = "/var/log/yc-iot/mqtt"
  mqtt_cloudlogs_path = "${local.mqtt_logs_dir}/cloud-logs.log"
  mqtt_accesslog_path = "${local.mqtt_logs_dir}/access.log"
  mqtt_self_ping_certs_dir = "/etc/yc-iot/mqtt/self-ping"
  mqtt_logging_secret = "/etc/yandex/statbox-push-client/logging_secret"
}

data "template_file" "mqtt_name" {
  count    = var.instance_count
  template = "$${name}"
  vars = {
    name = "${local.mqtt_name_prefix}-${local.environment}-${local.mqtt_insances[count.index][local.zone_key]}${format("%02d", floor(1 + (count.index / length(var.zone_suffix))))}"
  }
}

data "template_file" "mqtt_user_data" {
  template = file("${path.module}/files/user-data.tpl.yaml")
  count = var.instance_count

  vars = {
    disk_id    = yandex_compute_disk.mqtt_disk[count.index].id
  }
}

data "template_file" "mqtt_yaml" {
  template = file("${path.module}/files/mqtt/mqtt.tmpl.yaml")
  count    = var.instance_count

  vars = {
    tvm_token = module.yav-secret-mqtt-tvm_token.value
    mqtt_id   = count.index
    mqtt_name = data.template_file.mqtt_name[count.index].rendered
    zone      = local.mqtt_insances[count.index][local.zone_key]
    cloudlog_log_file = local.mqtt_cloudlogs_path
    self_ping_certs = local.mqtt_self_ping_certs_dir
    log_dir = local.mqtt_logs_dir
  }
}

data "template_file" "mqtt_tvmtool_conf" {
  template = file("${path.module}/files/mqtt/tvmtool.tpl.conf")

  vars = {
    secret    = module.yav-secret-mqtt-tvm_secret.value
    client_id = module.yav-secret-mqtt-tvm_client_id.value
  }
}

data "template_file" "mqtt_push_client_conf" {
  template = file("${path.module}/files/mqtt/mqtt-push-client.tpl.yaml")

  vars = {
    cloudlog_log_file = local.mqtt_cloudlogs_path
    access_log_file = local.mqtt_accesslog_path
    secret_file     = local.mqtt_logging_secret
    client_id = module.yav-secret-mqtt-tvm_client_id.value
  }
}

data "template_file" "mqtt-configs" {
  template = file("${path.module}/files/mqtt/mqtt-configs.tpl")
  count    = var.instance_count

  vars = {
    yandex_internal_root_ca = file("${path.module}/../../common/allCAs.pem")
    mqtt_config             = element(data.template_file.mqtt_yaml.*.rendered, count.index)
    mqtt_cert               = local.mqtt_cert
    mqtt_key                = local.mqtt_key
    tvmtool_config          = data.template_file.mqtt_tvmtool_conf.rendered
    push_client_conf        = element(data.template_file.mqtt_push_client_conf.*.rendered, count.index)
    name                    = "mqtt_preprod_${count.index}"
    self_ping_certs         = local.mqtt_self_ping_certs_dir
    self_ping_pub_cert      = local.mqtt_self_ping_pub_cert
    self_ping_sub_cert      = local.mqtt_self_ping_sub_cert
    logging_secret_data     = module.yav-secret-mqtt-tvm_secret.value
  }
}

data "template_file" "mqtt_manifest" {
  template = file("${path.module}/files/mqtt/mqtt-pod.tpl.yaml")
  count    = var.instance_count

  vars = {
    config_digest = sha256(
      element(data.template_file.mqtt-configs.*.rendered, count.index),
    )
    infra_pod_spec      = module.mqtt-infrastructure-pod.infra-pod-spec-no-push-client
    tvmtool_auth_token = module.yav-secret-mqtt-tvm_token.value
    push_client_version = var.push_client_version
  }
}

resource "random_string" "cluster-token" {
  length  = 16
  special = false
  upper   = false
}

module "mqtt-infrastructure-pod" {
  source                       = "../../modules/infrastructure-pod-metrics-agent"
  push_client_conf_path        = "${path.module}/../../common-gateway/files/push-client/push-client.yaml"
  juggler_bundle_manifest_path = "${path.module}/../../modules/infrastructure-pod-metrics-agent/files/MANIFEST.json"
  platform_http_check_path     = "${path.module}/../../modules/infrastructure-pod-metrics-agent/files/platform-http-checks.json"
  metrics_agent_conf_path      = "${path.module}/files/mqtt/metrics-agent.yaml.tpl"
  solomon_shard_cluster = "preprod"
}

resource "yandex_compute_disk" "mqtt_disk" {
  count = var.instance_count
  name  = "${data.template_file.mqtt_name[count.index].rendered}-persistent"
  description = "${data.template_file.mqtt_name[count.index].rendered}-persistent"
  type  = "network-hdd"
  zone = local.zones[local.mqtt_insances[count.index][local.zone_key]]
  size = var.data_disk_size
  labels = {
    layer = "paas"
    abc_svc = "ycmqtt"
    env = "pre-prod"
    environment = local.environment
  }
}

resource "yandex_compute_instance" "mqtt-instance-group" {
  count       = var.instance_count
  name        = data.template_file.mqtt_name[count.index].rendered
  hostname    = data.template_file.mqtt_name[count.index].rendered
  description = data.template_file.mqtt_name[count.index].rendered

  service_account_id = var.mqtt_sa_id
  allow_stopping_for_update = true
  resources {
    cores  = var.mqtt_instance_cores
    memory = var.mqtt_instance_memory
  }

  boot_disk {
    initialize_params {
      image_id = var.image_id
      size     = var.instance_disk_size
    }
  }

  zone = local.zones[local.mqtt_insances[count.index][local.zone_key]]

  network_interface {
    subnet_id    = local.subnets[local.mqtt_insances[count.index][local.zone_key]]
    ipv6         = "true"
    ip_address   = local.mqtt_insances[count.index][local.ipv4_key]
    ipv6_address = local.mqtt_insances[count.index][local.ipv6_key]
  }

  secondary_disk {
    disk_id = element(yandex_compute_disk.mqtt_disk.*.id, count.index)
  }

  metadata = {
    configs            = element(data.template_file.mqtt-configs.*.rendered, count.index)
    podmanifest        = element(data.template_file.mqtt_manifest.*.rendered, count.index)
    docker-config      = data.template_file.docker_json.rendered
    ssh-keys           = module.ssh-keys.ssh-keys
    serial-port-enable = "1"
    infra-configs      = module.mqtt-infrastructure-pod.infra-pod-configs-no-push-client
    osquery_tag        = "ycloud-svc-iot"
    user-data          = element(data.template_file.mqtt_user_data.*.rendered, count.index)
  }

  labels = {
    role                 = local.mqtt_name_prefix
    environment          = local.environment
    version              = local.version
    zone                 = local.zones[local.mqtt_insances[count.index][local.zone_key]]
    cluster_id           = random_string.cluster-token.result
    skip_update_ssh_keys = "true"
    yandex-dns           = data.template_file.mqtt_name[count.index].rendered
    conductor-group      = local.mqtt_name_prefix
    conductor-role       = "${local.mqtt_name_prefix}-${local.environment}"
    conductor-dc         = local.mqtt_insances[count.index][local.zone_key]
  }
}

