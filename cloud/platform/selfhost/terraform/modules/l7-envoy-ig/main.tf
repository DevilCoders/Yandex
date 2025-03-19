terraform {
  required_providers {
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = ">= 0.53"
    }
    yandex = {
      source = "yandex-cloud/yandex"
    }
  }

  required_version = ">= 1"
}

locals {
  default_files = {
    "/juggler-bundle/platform-http-checks.json" = file("${path.module}/platform-http-checks.json")

    "/etc/l7/configs/envoy/ssl/certs/allCAs.pem" = file("${path.module}/allCAs.pem")
  }
}

resource "ycp_microcosm_instance_group_instance_group" "this" {
  lifecycle {
    prevent_destroy = true
  }

  depends_on = [
    var.module_depends_on,
  ]

  name = var.name
  labels = {
    "environment" = var.env
  }

  service_account_id = var.ig_sa

  instance_template {
    service_account_id = var.instance_sa

    labels = {
      "abc_svc"         = "ycl7"
      "env"             = (var.env2 != null ? var.env2 : var.env) # dunno why, but it's "pre-prod"
      "environment"     = var.env
      "layer"           = "paas"
      "conductor-group" = "l7-${var.conductor_group_suffix}"
      "yandex-dns"      = "ig"
    }

    platform_id = var.platform

    resources {
      memory        = var.memory
      cores         = var.cores
      core_fraction = var.core_fraction
    }

    metadata = {
      "environment"       = var.env
      "internal-hostname" = "${var.name}-${var.env}-{instance.internal_dc}{instance.index_in_zone}"
      "internal-name"     = "${var.name}-${var.env}-{instance.internal_dc}{instance.index_in_zone}"
      "yandex-dns"        = "${var.name}-${var.env}-{instance.internal_dc}{instance.index_in_zone}"
      "nsdomain"          = "{instance.internal_dc}.ycp.${var.domain}"
      "osquery_tag"       = "ycloud-svc-l7-config"

      "ma_cluster"        = "cloud_${var.env}_${var.ma_name}"
      "kms-endpoint"      = var.kms_endpoint
      "push-client-ident" = var.push_client_ident

      "internal-remove-target-after-stop" = "true"

      "solomon_token_kms" = var.solomon_token

      "k8s-runtime-bootstrap-yaml" = var.k8s_bootstrap

      "skm" = jsonencode({
        "api_endpoint"  = "localhost:666"
        "kms_key_uri"   = "yc-kms://none"
        "encrypted_dek" = base64encode("none")
      })

      "user-data" = (var.user_data != null ? var.user_data : file("${path.module}/user_data.yaml"))

      "ssh-keys" = (var.ssh_keys != null ? var.ssh_keys : file("${path.module}/default_ssh_keys.txt"))

      "envoy_config" = templatefile("${path.module}/envoy.tpl.yaml", {
        "cluster"         = var.alb_lb_id
        "enable_tracing"  = var.enable_tracing
        "tracing_service" = var.tracing_service
        "xds_endpoints"   = var.xds_endpoints
        "xds_sni"         = var.xds_sni
        "remote_als_addr" = var.remote_als_addr
        "als_addr"        = var.als_addr
        "als_port"        = var.als_port
        "xds_auth"        = var.xds_auth

        "frontend_cert"   = var.frontend_cert
        "xds_client_cert" = var.xds_client_cert
      })

      "config-dumper-endpoints" = jsonencode([
        for addr in var.xds_endpoints : "[${addr}]:${var.xds_dumps_port}"
      ])

      "files" = jsonencode(merge(local.default_files, var.files))

      "dummy" = var.dummy

      ######################################
      # Deprecated by "files":

      "server_cert_crt"     = var.server_cert_pem
      "server_cert_key_kms" = var.server_cert_key

      "client_cert_crt"     = var.client_cert_pem
      "client_cert_key_kms" = var.client_cert_key

      "platform-http-checks" = file("${path.module}/platform-http-checks.json")

      "ca-bundle" = (var.ca_bundle != null ? var.ca_bundle : file("${path.module}/allCAs.pem"))

      # SDS
      "sds-log-level" = var.sds_log_level
      "sds-endpoints" = jsonencode({
        "alb"                 = var.alb_endpoint
        "certificate_manager" = var.cert_manager_endpoint
      })
      "sds-enable-cert-mgr" = var.sds_enable_cert_manager
    }

    boot_disk {
      mode = "READ_WRITE"

      disk_spec {
        image_id = var.image_id
        size     = var.boot_disk_size
        type_id  = "network-hdd"
      }
    }

    secondary_disk {
      device_name = "data"
      mode        = "READ_WRITE"

      disk_spec {
        size    = var.secondary_disk_size
        type_id = "network-hdd"
      }
    }

    network_settings {
      type = var.network_type
    }

    network_interface {
      network_id = var.network_id
      subnet_ids = var.subnet_ids

      primary_v4_address {
        name = "ig-v4addr"
      }

      primary_v6_address {
        name = "ig-v6addr"
      }
    }

    scheduling_policy {
      termination_grace_period = "600s"
    }
  }

  scale_policy {
    fixed_scale {
      size = var.size
    }
  }

  deploy_policy {
    max_creating     = 9
    max_deleting     = 1
    max_unavailable  = 1
    startup_duration = "0s"
  }

  allocation_policy {
    dynamic "zone" {
      for_each = toset(var.zones)
      content {
        zone_id = zone.value
      }
    }
  }

  load_balancer_spec {
    target_group_spec {
      name          = (var.optional_tg_name != null ? var.optional_tg_name : var.name)
      address_names = (var.has_ipv4 ? ["ig-v6addr", "ig-v4addr"] : ["ig-v6addr"])
    }
  }
}
