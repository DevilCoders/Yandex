resource "ycp_compute_instance" "dns_proxy" {
  count       = var.vm_count
  name        = "${var.prefix}-dns-proxy-${var.suffix}${count.index + 1}"
  hostname    = "${var.prefix}-dns-proxy-${var.suffix}${count.index + 1}"
  description = "PrivateCloud-to-Yandex DNS Proxy, ${var.prefix}-${var.suffix}${count.index + 1}"
  zone_id     = var.zone_id
  cauth       = true
  folder_id   = var.folder_id
  fqdn        = "${var.prefix}-dns-proxy-${var.suffix}${count.index + 1}.${var.dns_zone}"
  platform_id = "standard-v2"
  pci_topology_id = "V2"

  placement_policy {
    placement_group_id = var.placement_group_id
  }

  resources {
    cores  = 2
    memory = 4
    core_fraction = 100
  }

  boot_disk {
    disk_spec {
      size     = 20
      image_id = var.image_id
    }
  }

  network_interface {
    subnet_id = var.dmzprivcloud_subnet_id
    primary_v4_address {}
    primary_v6_address {
      address = element(var.dmzprivcloud_addresses, count.index)
    }
  }

  network_interface {
    subnet_id = var.interconnect_subnet_id
    primary_v6_address {
      address = element(var.interconnect_addresses, count.index)
    }
  }

  labels = {
    hostname = "${var.prefix}-dns-proxy-${var.suffix}${count.index + 1}.${var.dns_zone}"
  }

  metadata = {
    user-data = templatefile("${path.module}/cloud-init.yaml", { hostname = "${var.prefix}-dns-proxy-${var.suffix}${count.index + 1}.${var.dns_zone}" })
  }
}
