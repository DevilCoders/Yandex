locals {
  ssh_key = file(var.ssh_key)
}

data "template_file" "xrdp" {
  template = file("${path.module}/scripts/init.yaml")
  vars = {
    username = var.username
    password = var.password
    ssh_key  = local.ssh_key
  }
}

data "yandex_compute_image" "xrdp" {
  family    = var.image_family
  folder_id = var.folder_id
}

resource "yandex_compute_instance" "xrdp" {
  name        = local.name
  hostname    = local.name
  platform_id = "standard-v2"
  zone        = var.zone

  resources {
    cores  = var.cores
    memory = var.memory
  }

  boot_disk {
    initialize_params {
      image_id = data.yandex_compute_image.xrdp.id
      size     = var.disk_size
      type     = var.disk_type
    }
  }

  network_interface {
    subnet_id          = yandex_vpc_subnet.xrdp.id
    nat                = var.nat
    security_group_ids = [yandex_vpc_security_group.xrdp.id]
  }

  metadata = {
    user-data = data.template_file.xrdp.rendered
  }
}
