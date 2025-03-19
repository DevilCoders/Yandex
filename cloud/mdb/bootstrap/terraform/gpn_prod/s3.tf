locals {
  hosts = flatten([
    for type, cluster in tomap(var.s3.cluster) : [
      for shard in range(cluster.shards) : [
        for host_num in range(cluster.shard_hosts_per_zone) : [
          for zone, suffix in var.s3.zones : {
            type              = type
            shard             = format("%02d", shard + 1)
            host_num          = format("%02d", host_num + 1)
            zone              = zone
            suffix            = suffix
            name              = format("%s%02d-%02d-%s", cluster.prefix, shard + 1, host_num + 1, suffix)
            fqdn              = format("%s%02d-%02d-%s.%s", cluster.prefix, shard + 1, host_num + 1, suffix, var.s3.domain)
            datadir           = cluster.datadir
            nvme_disks        = cluster.nvme_disks
            cores             = cluster.cores
            memory_gb         = cluster.memory_gb
            boot_disk_size_gb = cluster.boot_disk_size_gb
            platform          = cluster.platform
          }
        ]
      ]
    ]
  ])
}

data "local_file" "ca" {
  filename = "${path.module}/gpnYandexCAs.pem"
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
    cafile     = indent(6, data.local_file.ca.content)
  }
}

resource "ycp_compute_instance" "s3" {

  for_each = {
    for host in local.hosts : "${host.type}${host.shard}-${host.host_num}-${host.suffix}" => host
  }

  provider = ycp.s3

  fqdn        = each.value.fqdn
  name        = each.value.name
  zone_id     = each.value.zone
  platform_id = each.value.platform

  boot_disk {
    disk_spec {
      image_id = "d8ojv16ms626r5dfrpqv"
      labels   = {}
      size     = each.value.boot_disk_size_gb
      type_id  = "network-ssd"
    }
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
