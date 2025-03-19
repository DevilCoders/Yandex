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
    subnet_id    = "e9bigio0vhk246euavkb"
    ip_address   = "172.16.0.69"
    ipv6_address = "2a02:6b8:c0e:500:0:f81c:0:aaaa"
  }

  resources {
    cores  = 1
    memory = 2
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
    subnet_id    = "e2l36n63vhg7vg6b9j8r"
    ip_address   = "172.17.0.69"
    ipv6_address = "2a02:6b8:c02:900:0:f81c:0:bbbb"
  }

  resources {
    cores  = 1
    memory = 2
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
    subnet_id    = "b0crolik07ik4hsqg543"
    ip_address   = "172.18.0.69"
    ipv6_address = "2a02:6b8:c03:500:0:f81c:0:cccc"
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
  # name         = "yc compute"
  source       = "../../../selfhost/terraform/modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "yccompute2"
}

module "ssh-keys-security" {
  # name         = "ycsecurity"
  source       = "../../../selfhost/terraform/modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "ycsecurity"
}