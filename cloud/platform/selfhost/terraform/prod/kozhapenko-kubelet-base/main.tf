provider "yandex" {
  endpoint  = "${var.yc_endpoint}"
  token     = "${var.yc_token}"
  folder_id = "${var.yc_folder}"
  zone      = "${var.yc_zone}"
}

module "ssh-keys" {
  source       = "../../modules/ssh-keys"
  yandex_token = "${var.yandex_token}"
  abc_service  = "cloud-platform"
}

module "infrastructure-pod" {
  source                = "../../modules/infrastructure-pod-metrics-agent"
  juggler_check_name    = "envoy-healthcheck"
  solomon_shard_cluster = "kozhapenko_kubelet_test"
  is_gateway            = true
  logcleaner_files_to_keep = "2"
}

data "template_file" "docker_json" {
  template = "${file("${path.module}/../../common-gateway/files/docker-json.tpl")}"

  vars = {
    docker_auth = "${var.docker_auth}"
  }
}

data "template_file" "pod_manifest" {
  template = "${file("${path.module}/files/test-pod.yaml.tpl")}"

  vars = {
    config_digest = "${sha256(data.template_file.configs.rendered)}"
    infra_pod_spec = "${module.infrastructure-pod.xds-infra-pod-spec-no-fluentd}"
  }
}

data "template_file" "configs" {
  template = "${file("${path.module}/files/configs.tpl")}"

  vars = {
    als_config          = "${file("${path.module}/../../common-gateway/files/common-als.yaml")}"
    yandex_internal_root_ca  = file("${path.module}/../../common/allCAs.pem")
    envoy_config        = "${file("${path.module}/files/envoy.yaml")}"
    gateway_config = file(
    "${path.module}/../../common-gateway/files/common-gateway.yaml",
    )
    envoy_cert          = local.envoy_cert
    envoy_key           = local.envoy_key
    configserver_config = "${file("${path.module}/../../common-gateway/files/common-configserver.yaml")}"
    envoy_resources     = "${file("${path.module}/files/envoy-resources.yaml")}"
    gateway_services    = "${file("${path.module}/files/gateway-services.yaml")}"
  }
}

module "kozhapenko-kubelet-base-instance-group" {
  source          = "../../modules/kubelet_instance_group"
  name_prefix     = "kozhapenko-kubelet-base"
  hostname_prefix = "kozhapenko-kubelet-base"
  role_name       = "gateway"

  instance_group_size = "1"

  cores_per_instance  = "${var.instance_cores}"
  memory_per_instance = "${var.instance_memory}"
  disk_per_instance   = "${var.instance_disk_size}"
  image_id            = "${var.image_id}"

  configs              = "${data.template_file.configs.rendered}"
  infra-configs        = "${module.infrastructure-pod.infra-pod-configs-no-fluentd}"
  podmanifest          = "${data.template_file.pod_manifest.rendered}"
  docker-config        = "${data.template_file.docker_json.rendered}"
  ssh-keys             = "${module.ssh-keys.ssh-keys}"
  skip_update_ssh_keys = "false"

  subnets = "${var.subnets}"
  zones   = "${var.zones}"
}
