locals {
  app_name    = "filestore-server"
  name_suffix = "{instance.internal_dc}{instance.index_in_zone}"
  domain      = "svc.cloud-preprod.yandex.net"
  env         = "preprod"

  arc_dir       = "${path.root}/../../../../.."
  kikimr_config = "${local.arc_dir}/kikimr/production/configs/yandex-cloud/${local.env}/${var.zone}/nfs/cfg"

  module_config = "${path.module}/config"
}

data "yandex_compute_image" "image" {
  name      = "yandex-cloud-${local.app_name}-${var.image_version}"
  folder_id = var.folder_id
}

resource "ycp_microcosm_instance_group_instance_group" "group" {
  name = "${local.app_name}-${var.zone}"

  folder_id          = var.folder_id
  service_account_id = var.service_account_id

  scale_policy {
    fixed_scale {
      size = 1
    }
  }

  allocation_policy {
    zone {
      zone_id = var.zone_ids[var.zone]
    }
  }

  deploy_policy {
    max_creating     = 0
    max_deleting     = 0
    max_expansion    = 0
    max_unavailable  = 1
    startup_duration = "120s"
  }

  instance_template {
    name     = "${local.app_name}-${local.name_suffix}"
    hostname = "${local.app_name}-${local.name_suffix}"
    fqdn     = "${local.app_name}-${local.name_suffix}.${local.domain}"

    platform_id        = "standard-v2"
    service_account_id = var.instance_service_account_id

    resources {
      cores         = 2
      core_fraction = 50
      memory        = 8 // GB
    }

    boot_disk {
      device_name = "system"
      mode        = "READ_WRITE"

      disk_spec {
        type_id  = "network-hdd"
        size     = 10 // GB
        image_id = data.yandex_compute_image.image.image_id
      }
    }

    secondary_disk {
      device_name = "logs"
      mode        = "READ_WRITE"

      disk_spec {
        type_id = "network-hdd"
        size    = 100 // GB
      }
    }

    # underlay_network {
    #   network_name = "underlay-v6"

    #   ipv6_dns_record_spec {
    #     fqdn        = "${local.app_name}-${local.name_suffix}"
    #     dns_zone_id = var.dns_zone_id
    #   }
    # }

    network_interface {
      subnet_ids = [
        for name, value in var.zone_ids :
        var.subnet_ids[name]
      ]

      primary_v4_address {}

      primary_v6_address {
        dns_record_spec {
          fqdn        = "${local.app_name}-${local.name_suffix}"
          dns_zone_id = var.dns_zone_id
        }
      }
    }

    labels = {
      abc_svc     = "ycnbs"
      environment = local.env
      version     = "${local.app_name}-${var.image_version}"
    }

    metadata = {
      shortname      = "${local.app_name}-${local.name_suffix}"
      nsdomain       = local.domain
      osquery_tag    = "ycloud-svc-${local.app_name}"
      ssh-keys       = "" // managed externally
      enable-oslogin = true

      user-data                  = file("${local.module_config}/cloud-init.yaml")
      k8s-runtime-bootstrap-yaml = file("${local.module_config}/k8s-runtime-bootstrap.yaml")
      skm                        = file("${local.module_config}/skm.yaml")
      application-config         = file("${local.module_config}/app.yaml")
      jaeger-agent-config        = file("${local.module_config}/jaeger-agent.yaml")

      nfs-server-cfg = file("${local.kikimr_config}/nfs_server.cfg")
      nfs-diag       = file("${local.kikimr_config}/nfs-diag.txt")
      nfs-domains    = file("${local.kikimr_config}/nfs-domains.txt")
      nfs-ic         = file("${local.kikimr_config}/nfs-ic.txt")
      nfs-log        = file("${local.kikimr_config}/nfs-log.txt")
      nfs-names      = file("${local.kikimr_config}/nfs-names.txt")
      nfs-server     = file("${local.kikimr_config}/nfs-server.txt")
      nfs-storage    = file("${local.kikimr_config}/nfs-storage.txt")
      nfs-sys        = file("${local.kikimr_config}/nfs-sys.txt")
    }
  }

  lifecycle {
    ignore_changes = [
      instance_template.0.metadata["ssh-keys"]
    ]
  }
}
