provider "yandex" {
  endpoint = var.yc_endpoint
  token = var.yc_token
  folder_id = var.yc_folder
  zone = var.yc_zone
}

module "ssh-keys" {
  source = "../../modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service = "cloud-platform"
}

module "infrastructure-pod" {
  source = "../../modules/infrastructure-pod-metrics-agent"
  solomon_shard_cluster = "cloud_preprod_jaegermeister"
  juggler_check_name = "jaegermeister-prom"
  platform_http_check_path = "${path.module}/files/platform-http-checks.json"
  metrics_agent_conf_path = "${path.module}/files/metrics-agent.yaml.tpl"
}

data "template_file" "jaegermeister_name" {
  count = var.instance_count
  template = "$${name}"

  vars = {
    name = "jaegermeister-preprod-${var.zone_suffix[element(var.zones, count.index)]}${format(
      "%02d",
      floor(count.index % var.instance_count / length(var.zone_suffix)) + 1,
    )}"
  }
}

//data "template_file" "jaegermeister_yaml" {
//  count = var.instance_count
//  template = file("${path.module}/files/jaegermeister.tmpl.yaml")
//
//  vars = {
//    id = element(data.template_file.jaegermeister_yaml.*.rendered, count.index)
//    region = var.zone_regions[element(var.zones, count.index)]
//    zone = element(var.zones, count.index)
//  }
//}

data "template_file" "configs" {
  count = var.instance_count
  template = file("${path.module}/files/configs.tpl")

  vars = {
    yandex_internal_root_ca = file("${path.module}/../../common/allCAs.pem")
    jaegermeister_config = file("${path.module}/files/jaegermeister.yaml")
  }
}

data "template_file" "docker_json" {
  template = file("${path.module}/files/docker.json.tpl")

  vars = {
    docker_auth = local.docker_auth
  }
}

data "template_file" "pod_manifest" {
  count = var.instance_count
  template = file("${path.module}/files/jaegermeister-pod.tpl.yaml")

  vars = {
    aws_access_key = local.aws_access_key
    aws_secret_key = local.aws_secret_key
    config_digest = sha256(element(data.template_file.configs.*.rendered, count.index))
    infra_pod_spec = module.infrastructure-pod.infra-pod-spec-no-push-client
  }
}

resource "yandex_compute_instance" "jaegermeister" {
  count = var.instance_count

  name = element(data.template_file.jaegermeister_name.*.rendered, count.index)
  hostname = element(data.template_file.jaegermeister_name.*.rendered, count.index)
  description = var.instance_description
  allow_stopping_for_update = true

  resources {
    cores = var.instance_cores
    memory = var.instance_memory
  }

  boot_disk {
    initialize_params {
      image_id = var.image_id
      size = var.instance_disk_size
    }
  }

  zone = element(var.zones, count.index)

  network_interface {
    subnet_id = var.zone_subnets[element(var.zones, count.index)]
    ip_address = element(var.ipv4_addrs, count.index)
    ipv6_address = element(var.ipv6_addrs, count.index)
  }

  metadata = {
    configs = element(data.template_file.configs.*.rendered, count.index)
    infra-configs = module.infrastructure-pod.infra-pod-configs-no-push-client
    podmanifest = element(data.template_file.pod_manifest.*.rendered, count.index)
    docker-config = data.template_file.docker_json.rendered
    yandex-dns = element(data.template_file.jaegermeister_name.*.rendered, count.index)
    ssh-keys = module.ssh-keys.ssh-keys
    osquery_tag = "ycloud-svc-jaegermeister"
  }

  labels = {
    role = "jaegermeister"
    cluster_id = random_string.cluster-token.result
    skip_update_ssh_keys = var.skip_update_ssh_keys
    yandex-dns = element(data.template_file.jaegermeister_name.*.rendered, count.index)
    conductor-group = "jaegermeister"
    conductor-dc = var.zone_suffix[element(var.zones, count.index)]
    layer = "paas"
    abc_svc = "cloud-platform-infra"
    env = "pre-prod"
  }
}

resource "random_string" "cluster-token" {
  length = 16
  special = false
  upper = false
}

