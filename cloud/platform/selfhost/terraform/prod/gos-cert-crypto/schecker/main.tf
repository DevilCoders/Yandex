locals {
  instances = flatten([
  for i in range(var.instances_amount) : {
      hostname     = "${var.dns_prefix}-${var.yc_zone_suffixes[var.yc_zones[i % length(var.yc_zones)]]}-${i/length(var.yc_zones)+1}"
      fqdn         = "${var.dns_prefix}-${var.yc_zone_suffixes[var.yc_zones[i % length(var.yc_zones)]]}-${i/length(var.yc_zones)+1}.${var.dns_zone}"
      az           = var.yc_zones[i % length(var.yc_zones)]
      subnet_id    = var.subnets[var.yc_zones[i % length(var.yc_zones)]]
      ipv4_address = var.ipv4_addresses[i]
      ipv6_address = var.ipv6_addresses[i]
    }
  ])

  instances_map = {
  for instance in local.instances : replace(instance.hostname, ".", "-") => instance
  }
}

resource "ycp_dns_dns_record_set" "schecker_fqdn" {
  for_each = local.instances_map
  name     = each.value.hostname
  zone_id  = var.dns_zone_id
  type     = "AAAA"
  ttl      = 300
  data     = [each.value.ipv6_address]
}

resource "ycp_compute_instance" "schecker_host" {
  for_each    = local.instances_map
  zone_id     = each.value.az
  platform_id = var.platform_id
  name        = each.key
  fqdn        = each.value.fqdn
                    
  service_account_id = ycp_iam_service_account.schecker_sa.id

  placement_policy {
    host_group = "service"
  }

  resources {
    cores         = var.instance_cores
    core_fraction = var.instance_core_fraction
    memory        = var.instance_memory
  }

  boot_disk {
    disk_spec {
      name        = "${each.key}-boot-disk"
      description = "${each.key} boot disk"
      image_id    = var.image_id
      type_id     = var.instance_disk_type
      size        = var.instance_disk_size
      labels      = var.labels
    }
    device_name = "boot"
  }

  network_interface {
    subnet_id = each.value.subnet_id
    primary_v6_address {
      address = each.value.ipv6_address
    }
    primary_v4_address {
      address = each.value.ipv4_address
    }
    security_group_ids = var.security_group_ids
  }

  metadata = {
    user-data = templatefile("${path.module}/files/cloud-init.yaml", {
      fqdn = each.value.fqdn
      schecker-swiss-knife-config = templatefile("${path.module}/files/yc-schecker-swiss-knife.config.yaml", {
        db_host = var.database.host
        db_port = var.database.port
        db_name = var.database.name
        db_user = var.database.user

        smtp_addr = var.smtp.addr
        smtp_user = var.smtp.user
      })

      syncer-config = templatefile("${path.module}/files/yc-schecker-syncer.config.yaml", {
        db_host = var.database.host
        db_port = var.database.port
        db_name = var.database.name
        db_user = var.database.user
      })

      migrator-config = templatefile("${path.module}/files/yc-schecker-migrator.config.yaml", {
        db_host = var.database.host
        db_port = var.database.port
        db_name = var.database.name
        db_user = var.database.user
      })

      parser-config = templatefile("${path.module}/files/yc-schecker-parser.config.yaml", {
        db_host = var.database.host
        db_port = var.database.port
        db_name = var.database.name
        db_user = var.database.user
      })

    })

    skm = file("${path.module}/files/skm/skm.yaml")

    ssh-keys = module.ssh-keys.ssh-keys
    podmanifest = ""
  }

  labels = var.labels
}
