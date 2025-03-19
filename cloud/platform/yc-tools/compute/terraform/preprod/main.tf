data "yandex_compute_image" "deploy" {
  family    = "deploylander"
  folder_id = var.yc_folder
}

resource "yandex_compute_instance" "this_a" {
  zone = "ru-central1-a"
  boot_disk {
    initialize_params {
      image_id = data.yandex_compute_image.deploy.id
      size = 300
    }
  }

  network_interface {
    subnet_id    = "bucfs8b7ub3nbpu27s0s"
    ip_address   = "172.16.0.69"
    ipv6_address = "2a02:6b8:c0e:501:0:fc0b:0:aaaa"
  }

  resources {
    cores  = 2
    memory = 4
  }

  metadata = {
    ssh-keys = local.ssh-keys
  }
}

resource "yandex_compute_instance" "this_b" {
  zone = "ru-central1-b"
  boot_disk {
    initialize_params {
      image_id = data.yandex_compute_image.deploy.id
      size = 300
    }
  }

  network_interface {
    subnet_id    = "blt51409ua25dcueqlvi"
    ip_address   = "172.17.0.69"
    ipv6_address = "2a02:6b8:c02:901:0:fc0b:0:bbbb"
  }

  resources {
    cores  = 2
    memory = 4
  }

  metadata = {
    ssh-keys = local.ssh-keys
  }
}

resource "yandex_compute_instance" "this_c" {
  zone = "ru-central1-c"
  boot_disk {
    initialize_params {
      image_id = data.yandex_compute_image.deploy.id
      size = 300
    }
  }

  network_interface {
    subnet_id    = "fo2de2qbj1r9jljioijh"
    ip_address   = "172.18.0.69"
    ipv6_address = "2a02:6b8:c03:501:0:fc0b:0:cccc"
  }

  resources {
    cores  = 2
    memory = 4
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
  # name         = "yc compute"
  source       = "../../selfhost/terraform/modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "yccompute2"
}

module "ssh-keys-security" {
  # name         = "ycsecurity"
  source       = "../../selfhost/terraform/modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "ycsecurity"
}
