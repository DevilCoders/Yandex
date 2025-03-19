data "yandex_compute_image" "deploy" {
  family    = "deploylander"
  folder_id = var.yc_folder
}

resource "yandex_compute_instance" "this" {
  boot_disk {
    initialize_params {
      image_id = data.yandex_compute_image.deploy.id
    }
  }

  network_interface {
    subnet_id    = "fo2de2qbj1r9jljioijh"
    ip_address   = "172.18.0.23"
    ipv6_address = "2a02:6b8:c03:501:0:fc0b:0:beef"
    # make DNS record yourself
  }

  resources {
    cores  = 1
    memory = 2
  }

  metadata = {
    ssh-keys = local.ssh-keys
  }
}

locals {
  ssh-keys = join("\n",
    split("\n", module.ssh-keys-platform.ssh-keys),
    split("\n", module.ssh-keys-security.ssh-keys),
  )
}

module "ssh-keys-platform" {
  # name         = "cloud-platform"
  source       = "../../../../selfhost/terraform/modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "cloud-platform"
}

module "ssh-keys-security" {
  # name         = "ycsecurity"
  source       = "../../../../selfhost/terraform/modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "ycsecurity"
}
