locals {
  hosts = flatten([
    for type, cluster in tomap(var.s3.cluster) : [
      for shard in range(lookup(cluster, "shards_skip", 0), lookup(cluster, "shards_skip", 0) + cluster.shards) : [
        for zone, suffix in var.s3.zones : {
          type              = type
          shard             = format("%02d", shard + 1)
          zone              = zone
          suffix            = suffix
          name              = format("%s%02d%s", cluster.prefix, shard + 1, suffix)
          fqdn              = format("%s%02d%s.%s", cluster.prefix, shard + 1, suffix, var.s3.domain)
          datadir           = cluster.datadir
          nvme_disks        = cluster.nvme_disks
          cores             = cluster.cores
          memory_gb         = cluster.memory_gb
          boot_disk_size_gb = cluster.boot_disk_size_gb
          platform          = cluster.platform
          image             = lookup(cluster, "image", "fd83i5r5g44fjkdpuuva") # standard ubuntu-1804-lts-1541165525
        }
      ]
    ]
  ])
}

data "template_file" "metadata" {
  template = file("../scripts/s3_metadata.yaml")

  for_each = {
    for host in local.hosts : host.fqdn => host
  }

  vars = {
    fqdn       = each.value.fqdn
    datadir    = each.value.datadir
    deploy_api = var.s3.deploy_api
    nvme_disks = each.value.nvme_disks
    cafile     = ""
  }
}

resource "ycp_compute_instance" "s3" {

  for_each = {
    for host in local.hosts : "${host.type}${host.shard}${host.suffix}" => host
  }

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
  }

  provider = ycp.s3

  fqdn        = each.value.fqdn
  name        = each.value.name
  zone_id     = each.value.zone
  platform_id = each.value.platform

  boot_disk {
    disk_spec {
      image_id = each.value.image
      labels   = {}
      size     = each.value.boot_disk_size_gb
      type_id  = "network-hdd"
    }
  }

  placement_policy {
    host_group = "service"
  }

  resources {
    core_fraction = 100
    cores         = each.value.cores
    memory        = each.value.memory_gb
    nvme_disks    = each.value.nvme_disks
  }

  underlay_network {
    network_name = "underlay-v6"
  }

  metadata = {
    "serial-port-enable" = "1"
    "user-data"          = data.template_file.metadata[each.value.fqdn].rendered
  }
}
