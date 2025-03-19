data "yandex_compute_image" "deploy" {
  family    = "deploylander"
  folder_id = var.yc_folder
}

resource "yandex_compute_instance" "this" {
  name     = "deploy-cloud-yandex-net"
  hostname = "deploy-host"

  boot_disk {
    initialize_params {
      # image_id = data.yandex_compute_image.deploy.id
      image_id = ""
      size = 150

    }
  }
  secondary_disk {
    disk_id     = yandex_compute_disk.homes.id
    device_name = "home-dirs"
    auto_delete = false
  }
  secondary_disk {
    auto_delete = true
    device_name = "prev-boot-disk"
    disk_id     = "ef3i0chlus6koies26rb"
    mode        = "READ_WRITE"
  }

  network_interface {
    subnet_id    = "b0csn544n0bki7n7hfi1"
    ip_address   = "172.18.0.33"
    ipv6_address = "2a02:6b8:c03:500:0:f813:0:cccc" # 'deploy.cloud.yandex.net' DNS record
  }

  platform_id = "standard-v2"

  resources {
    cores  = 4
    memory = 8
  }

  metadata = {
    ssh-keys           = local.ssh-keys
    user-data          = file("${path.module}/files/cloud-init-secondary-disk.yaml")
    serial-port-enable = true
  }
}

resource "yandex_compute_disk" "homes" {
  name = "home-dirs"
  size = 512
  type = "network-ssd"
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
    split("\n", module.ssh-keys-core.ssh-keys),
    split("\n", module.ssh-keys-networkloadbalancer.ssh-keys),
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

module "ssh-keys-core" {
  # name         = "yccore"
  source       = "../../../selfhost/terraform/modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "yccore"
}

module "ssh-keys-networkloadbalancer" {
  # name         = "networkloadbalancer"
  source       = "../../../selfhost/terraform/modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "networkloadbalancer"
}
