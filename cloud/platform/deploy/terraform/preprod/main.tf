data "yandex_compute_image" "deploy" {
  family    = "deploylander"
  folder_id = var.yc_folder
}

resource "yandex_compute_instance" "this" {
  boot_disk {
    initialize_params {
      # image_id = data.yandex_compute_image.deploy.id
      size = 10
    }
  }

  secondary_disk {
    disk_id     = yandex_compute_disk.homes.id
    device_name = "home-dirs"
    auto_delete = false
  }
  allow_stopping_for_update = true
  network_interface {
    subnet_id    = "fo21vdq0r3urutr9b9k8"
    ip_address   = "172.18.0.23"
    ipv6_address = "2a02:6b8:c03:501:0:fc05:0:cccc"
    # 'deploy.cloud-preprod.yandex.net' DNS record
  }

  resources {
    cores  = 1
    memory = 2
  }

  metadata = {
    ssh-keys  = local.ssh-keys
    user-data = file("${path.module}/files/cloud-init-secondary-disk.yaml")
  }
}

resource "yandex_compute_disk" "homes" {
  size = 64
  name = "home-dirs"
  type = "network-hdd"
  zone = var.yc_zone
  labels = {
    type = "home-dirs"
  }
}

locals {
  ssh-keys = join("\n",
    split("\n", module.ssh-keys-platform.ssh-keys),
    split("\n", module.ssh-keys-security.ssh-keys),
    split("\n", module.ssh-keys-ui.ssh-keys),
    split("\n", module.ssh-keys-compute.ssh-keys),
    split("\n", module.ssh-keys-vpc.ssh-keys),
  )
}

module "ssh-keys-platform" {
  # name         = "cloud-platform"
  source       = "../../../selfhost/terraform/modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "cloud-platform"
}

module "ssh-keys-security" {
  # name         = "ycsecurity"
  source       = "../../../selfhost/terraform/modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "ycsecurity"
}

module "ssh-keys-ui" {
  # name         = "ui"
  source       = "../../../selfhost/terraform/modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "ui"
}

module "ssh-keys-compute" {
  # name         = "yccompute2"
  source       = "../../../selfhost/terraform/modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "yccompute2"
}

module "ssh-keys-vpc" {
  # name         = "ycvpc"
  source       = "../../../selfhost/terraform/modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "ycvpc"
}
