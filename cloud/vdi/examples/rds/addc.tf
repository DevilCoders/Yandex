locals {
  ad_instance_name = join("-", ["rds", random_string.launch_code.result, "addc"])
  ad_ip_address    = cidrhost(var.subnet_v4_cidr_block, 3)
  ad_dns_forwarder    = cidrhost(var.subnet_v4_cidr_block, 2)
}

resource "random_password" "recovery_password" {
  length      = 16
  min_lower   = 1
  min_numeric = 1
  min_upper   = 1
  special     = false
}

data "template_file" "addc" {
  template = file("${path.module}/scripts/addc.ps1")
  vars = {
    admin_password    = var.admin_pass
    recovery_password = random_password.recovery_password.result
    domain_name       = local.domain_name
    dns_forwarder     = local.ad_dns_forwarder
  }
}

resource "yandex_compute_instance" "ad" {
  name        = local.ad_instance_name
  hostname    = local.ad_instance_name
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
    subnet_id          = yandex_vpc_subnet.default.id
    ip_address         = local.ad_ip_address
    nat                = var.nat
    security_group_ids = [yandex_vpc_security_group.ad.id]
  }

  metadata = {
    user-data = data.template_file.init.rendered
    deploy    = data.template_file.addc.rendered
  }

  timeouts {
    create = var.timeout_create
    delete = var.timeout_delete
  }
}

output "recovery_password" {
  value     = random_password.recovery_password.result
  sensitive = true
}

output "domain_name" {
  value = local.domain_name
}