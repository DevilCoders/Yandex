locals {
  environment = "preprod"
  env_name    = "preprod"

  base_image_family    = "platform-base-kubelet-static"
  base_image_folder_id = "${var.base_image_folder_id}"
  base_image_id        = "${var.base_image_id}"

  instance_group_size = 3
  instance_core       = 2
  instance_mem        = 4
  instance_disk_size  = 40

  zones = [
    "ru-central1-a",
    "ru-central1-b",
    "ru-central1-c",
  ]
}

data "external" "secrets" {
  program = [
    "/bin/bash",
    "-c",
    "${path.module}/load-secrets.sh",
  ]

  query = {
    environment = "${local.environment}"
  }
}

provider "yandex" {
  endpoint  = "${var.yandex_api_endpoint}"
  token     = "${data.external.secrets.result.cloud_token}"
  cloud_id  = "${var.yandex_cloud_id}"
  folder_id = "${var.yandex_folder_id}"
}

data "yandex_compute_image" "platform_base_kubelet_image" {
  family    = "${local.base_image_family}"
  folder_id = "${local.base_image_folder_id}"
}

data "template_file" "docker_json" {
  template = "${file("${path.module}/files/docker-json.tpl")}"

  vars = {
    docker_auth = "${data.external.secrets.result.docker_auth}"
  }
}

data "template_file" "podmanifest" {
  template = "${file("${path.module}/files/podmanifest.tpl")}"

  vars = {
    image = "${var.image}"
  }
}

data "template_file" "adapter_config" {
  template = "${file("${path.module}/files/adapter.yaml.tpl")}"

  // TODO: should be template for config?
}

resource "yandex_compute_instance" "backend" {
  count    = "${local.instance_group_size}"
  name     = "api-adapter-${local.env_name}-${element(local.zones, count.index)}"
  hostname = "api-adapter-${local.env_name}-${element(local.zones, count.index)}"
  zone     = "${element(local.zones, count.index)}"

  //noinspection HCLUnknownBlockType
  resources {
    cores  = "${local.instance_core}"
    memory = "${local.instance_mem}"
  }

  //noinspection HCLUnknownBlockType
  boot_disk {
    //noinspection HCLUnknownBlockType
    initialize_params {
      image_id = "${local.base_image_id}"
      size     = "${local.instance_disk_size}"
    }
  }

  //noinspection HCLUnknownBlockType
  network_interface {
    subnet_id = "${lookup(var.subnets, element(local.zones, count.index))}"
    ipv6      = true
  }

  //noinspection HCLUnknownBlockType
  metadata = {
    docker-config = "${data.template_file.docker_json.rendered}"
    podmanifest   = "${data.template_file.podmanifest.rendered}"
    config        = "${data.template_file.adapter_config.rendered}"

    export_files = <<EOF
        config
    EOF
  }

  //noinspection HCLUnknownBlockType
  labels = {
    environment = "preprod"

    yandex-dns      = "api-adapter-${local.env_name}-${substr(element(local.zones, count.index),12,1)}-${var.zone_suffix[element(local.zones, count.index)]}"
    conductor-group = "api-adapter_tf"
    conductor-dc    = "${var.zone_suffix[element(local.zones, count.index)]}"
  }

  lifecycle {
    ignore_changes = [
      "metadata.ssh-keys",
    ]
  }
}
