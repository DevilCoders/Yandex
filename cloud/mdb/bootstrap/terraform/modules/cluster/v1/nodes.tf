locals {
  names_with_zones = flatten([
    for z, _ in var.installation.zones : [
      for i in range(1, var.node_count_per_zone + 1) : [
        {
          name    = format("%s%s%02d-%s", var.name, var.dash_after_name ? "-" : "", i, z)
          zone_id = z
      }]
    ]
  ])
  nodes = {
    for nz in local.names_with_zones :
    nz.name => {
      name     = nz.name
      fqdn     = trimsuffix(format("%s.%s", nz.name, data.ycp_dns_dns_zone.zone.zone), ".")
      fqdn_abs = format("%s.%s", nz.name, data.ycp_dns_dns_zone.zone.zone)
      zone_id  = nz.zone_id
    }
  }
}

resource "ycp_certificatemanager_certificate_request" "cert" {
  for_each = local.nodes

  challenge_type = "CHALLENGE_TYPE_UNSPECIFIED"
  domains        = [each.value.fqdn]
  cert_provider  = "INTERNAL_CA"
}

data "template_file" "metadata" {
  template = file("${path.module}/metadata.yaml")

  for_each = local.nodes

  vars = {
    fqdn                        = each.value.fqdn
    deploy_api                  = var.installation.services.deploy_api
    lockbox_address             = var.installation.services.lockbox
    certificate_manager_address = var.installation.services.certificate_manager
    encoded_ca_list             = jsonencode(var.ca)
  }
}

data "ycp_dns_dns_zone" "zone" {
  dns_zone_id = var.installation.dns_zone_id
}

resource "ycp_compute_placement_group" "placement_group" {
  lifecycle {
    prevent_destroy = true
  }

  name      = "${var.name}-placement-group"
  folder_id = var.installation.folder_id
  spread_placement_strategy {
    best_effort            = false
    max_instances_per_node = 1
  }
}

resource "ycp_iam_service_account" "service_account" {
  lifecycle {
    prevent_destroy = true
  }

  folder_id          = var.installation.folder_id
  service_account_id = "${var.installation.service_account_prefix}.${var.name}-cluster"
  name               = "${var.name}-service-account"
  description        = "Service account for ${var.name} cluster"
}


resource "ycp_resource_manager_folder_iam_member" "service_account_get_secret_payload" {
  folder_id = var.installation.folder_id
  member    = "serviceAccount:${ycp_iam_service_account.service_account.id}"
  role      = "lockbox.payloadViewer"
  count     = var.can_get_all_secrets_in_folder ? 1 : 0
}

resource "ycp_resource_manager_folder_iam_member" "service_account_get_certificates" {
  folder_id = var.installation.folder_id
  member    = "serviceAccount:${ycp_iam_service_account.service_account.id}"
  role      = "certificate-manager.certificates.downloader"
  count     = var.can_get_all_secrets_in_folder ? 1 : 0
}


resource "ycp_storage_bucket" "backups" {
  bucket = "${var.installation.bucket_prefix}${var.name}-backups"
  count  = var.backups_bucket ? 1 : 0
}

resource "ycp_resource_manager_folder_iam_member" "service_account_get_salt_srv" {
  folder_id = var.installation.folder_id
  member    = "serviceAccount:${ycp_iam_service_account.service_account.id}"
  role      = "storage.viewer"
}

resource "ycp_compute_instance" "node" {
  for_each = local.nodes

  lifecycle {
    ignore_changes = [
      boot_disk[0].disk_spec[0].image_id,
      gpu_settings,
    ]
  }

  name                      = each.value.name
  zone_id                   = each.value.zone_id
  platform_id               = var.platform_id
  fqdn                      = each.value.fqdn
  allow_stopping_for_update = var.allow_stopping_for_update
  folder_id                 = var.installation.folder_id
  service_account_id        = ycp_iam_service_account.service_account.id

  resources {
    core_fraction = var.resources.core_fraction
    cores         = var.resources.cores
    memory        = var.resources.memory
  }

  boot_disk {
    auto_delete = true
    disk_spec {
      image_id = var.boot_disk.image_id
      size     = var.boot_disk.size
      type_id  = var.boot_disk.type_id
    }
  }

  network_interface {
    subnet_id = var.installation.zones[each.value.zone_id].subnet_id
    primary_v6_address {
      dns_record {
        fqdn        = each.value.fqdn_abs
        dns_zone_id = data.ycp_dns_dns_zone.zone.dns_zone_id
        ptr         = true
      }
    }
    primary_v4_address {}
  }

  placement_policy {
    placement_group_id = ycp_compute_placement_group.placement_group.id
    host_group         = "service"
  }


  metadata = {
    "serial-port-enable" = "1"
    "user-data"          = data.template_file.metadata[each.value.name].rendered
    "certificate-id"     = ycp_certificatemanager_certificate_request.cert[each.value.name].id
  }
}
