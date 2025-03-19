locals {
  // Build list of target instances
  instances = flatten([
    for zone in [for zone_name, zone in var.installation.known_zones : zone_name if length(var.zones) == 0 || contains(var.zones, zone_name)] : [
      for i in range(var.instances_per_zone) : {
        cluster = format("%02d", i + 1) // This is used only for resource address within terraform itself
        zone    = zone
        shortname = format(
          "%s%s%s%s",
          var.override_shortname_prefix != null ? var.override_shortname_prefix : "mdb-${var.service_name}",
          var.override_instance_env_suffix_with_emptiness ? "" : var.installation.instance_env_suffix,
          format("%02d", i + 1),
          var.override_zone_shortname_with_letter ? var.installation.known_zones[zone].letter : "-${var.installation.known_zones[zone].shortname}"
        )
      }
    ]
  ])
}

resource "ycp_compute_instance" "service_instance" {
  for_each = {
    for instance in local.instances : "${instance.cluster}.${instance.zone}" => instance
  }

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = true
  }

  name                      = each.value.shortname
  zone_id                   = var.installation.known_zones[each.value.zone].id
  platform_id               = var.platform_id
  fqdn                      = "${each.value.shortname}.${var.installation.dns_zone}"
  allow_stopping_for_update = var.allow_stopping_for_update
  disable_seccomp           = var.disable_seccomp
  pci_topology_id           = var.pci_topology_id

  resources {
    core_fraction = var.resources.core_fraction
    cores         = var.resources.cores
    memory        = var.resources.memory
  }

  boot_disk {
    auto_delete = try(var.override_boot_disks[each.key].auto_delete, true)
    disk_spec {
      name        = try(var.override_boot_disks[each.key].name, null)
      size        = try(var.override_boot_disks[each.key].size, var.boot_disk_spec.size)
      image_id    = try(var.override_boot_disks[each.key].image_id, var.boot_disk_spec.image_id)
      snapshot_id = try(var.override_boot_disks[each.key].snapshot_id, null)
      type_id     = try(var.override_boot_disks[each.key].type, var.boot_disk_spec.type_id)
    }
  }

  network_interface {
    subnet_id = var.installation.known_zones[each.value.zone].subnet_id
    primary_v6_address {}
    security_group_ids = length(var.override_security_groups) > 0 ? var.override_security_groups : var.installation.security_groups
  }

  // This code is from gpn prod. It enables provisioners but is not enabled and does not support
  // configuration. We are planning to move towards cloud-init with metadata so this code is deemed
  // unneeded. Still it is saved here for the time being. Remove this code when cloud-init/metadata
  // stuff is sorted. Or fix and enable it if for some reason we decide to abandon cloud-init/metadata.
  /*  provisioner "file" {
    source      = "../scripts/provision_host.py"
    destination = "/root/provision_host.py"
    connection {
      type = "ssh"
      user = "root"
      host = self.network_interface[0].primary_v6_address[0].address
    }
  }
  provisioner "remote-exec" {
    inline = [
      "chmod +x /root/provision_host.py",
      "/root/provision_host.py --fqdn=${self.fqdn} --token=${var.mdb_admin_token} --deploy=${var.mdb_deploy_api}"
    ]
    connection {
      type = "ssh"
      user = "root"
      host = self.network_interface[0].primary_v6_address[0].address
    }
  }*/
}

module "load_balancer" {
  source   = "../../load_balancer/v1"
  for_each = var.create_load_balancer ? toset(["mdb-${var.service_name}"]) : toset([])

  load_balancer_name = "mdb-${var.service_name}"
  region_id          = var.installation.region_id
  ipv6_addr          = var.override_load_balancer_ipv6_addr
  targets = [
    for zone, instance in ycp_compute_instance.service_instance : {
      address   = instance.network_interface.0.primary_v6_address[0].address
      subnet_id = instance.network_interface.0.subnet_id
    }
  ]
}

terraform {
  required_version = ">= 0.14.7, < 0.15.0"
  required_providers {
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = ">=0.32.0"
    }
  }
}
