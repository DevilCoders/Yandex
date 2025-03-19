locals {
  rdls_instance_name = join("-", ["rds", random_string.launch_code.result, "rdls"])
}

data "template_file" "rdls" {
  template = file("${path.module}/scripts/rdls.ps1")
  vars = {
    admin_password   = var.admin_pass
    domain_name      = local.realm_name
  }
}

resource "yandex_compute_instance" "rdls" {
  name        = local.rdls_instance_name
  hostname    = local.rdls_instance_name
  platform_id = "standard-v2"
  zone        = var.zone

  resources {
    cores  = var.cores
    memory = var.memory
  }

  boot_disk {
    initialize_params {
      image_id = data.yandex_compute_image.default.id
      size     = var.disk_size
      type     = var.disk_type
    }
  }

  network_interface {
    subnet_id          = yandex_vpc_subnet.rds.id
    nat                = var.nat
    security_group_ids = [yandex_vpc_security_group.rds.id]
  }

  metadata = {
    user-data = data.template_file.init.rendered
    deploy    = data.template_file.rdls.rendered
  }

  timeouts {
    create = var.timeout_create
    delete = var.timeout_delete
  }
}
