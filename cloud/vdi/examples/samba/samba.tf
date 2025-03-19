locals {
  samba_instance_name = join("-", ["rds", random_string.launch_code.result, "samba"])
  samba_instance_fqdn = join(".", [local.samba_instance_name, local.realm_name])
  samba_ip_address    = cidrhost(var.samba_v4_cidr_block, 3)
  samba_dns_forwarder = cidrhost(var.samba_v4_cidr_block, 2)
}

data "yandex_compute_image" "samba" {
  family = "ubuntu-2004-lts"
}

data "template_file" "samba" {
  template = file("${path.module}/scripts/samba.yaml")
  vars = {
    ssh_user      = var.ssh_user
    ssh_key       = "${file("~/.ssh/id_rsa.pub")}"
    ip            = local.samba_ip_address
    admin_pass    = var.admin_pass
    dns_forwarder = local.samba_dns_forwarder
    instance_name = local.samba_instance_name
    instance_fqdn = local.samba_instance_fqdn
    realm_name    = local.realm_name  # fqdn  -> rds-asdfg.local
    domain_name   = local.domain_name # short -> rds-asdfg
  }
}

resource "yandex_compute_instance" "samba" {
  name        = local.samba_instance_name
  hostname    = local.samba_instance_name
  platform_id = "standard-v2"
  zone        = var.zone

  resources {
    cores  = var.cores
    memory = var.memory
  }

  boot_disk {
    initialize_params {
      image_id = data.yandex_compute_image.samba.id
      size     = var.disk_size
      type     = var.disk_type
    }
  }

  network_interface {
    subnet_id          = yandex_vpc_subnet.samba.id
    ip_address         = local.samba_ip_address
    nat                = var.nat
    security_group_ids = [yandex_vpc_security_group.samba.id]
  }

  metadata = {
    user-data = data.template_file.samba.rendered
  }

  timeouts {
    create = var.timeout_create
    delete = var.timeout_delete
  }
}

output "realm_name" {
  value = local.realm_name
}