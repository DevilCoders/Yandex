// Deploy SA - used to deploy ig
resource "ycp_iam_service_account" "monops_deploy_sa" {
  lifecycle {
    prevent_destroy = true
  }

  name        = "monops-deploy"
  description = "Service account to deploy monops-web instance group"
  folder_id   = var.folder_id
}

// Used for:
//  - to access session service for user authentication in monops
//  - to encrypt/decrypt kms key
resource "ycp_iam_service_account" "monops_web_sa" {
  lifecycle {
     prevent_destroy = true
  }

  name        = "monops-web"
  description = "Service account for monops-web"
  folder_id   = var.folder_id
}

resource "yandex_kms_symmetric_key" "monops_kms_key" {
  lifecycle {
    prevent_destroy = true
  }

  name              = "monops-kms-key"
  description       = "Key for deciphering monops web tool secrets"
  default_algorithm = "AES_256"
  rotation_period   = "8760h" // equal to 1 year
  folder_id         = var.folder_id
}

resource "ycp_compute_image" "monops_image" {
  description   = "monops web image"
  folder_id     = var.folder_id
  name          = "yc-monops-web"
  uri           = "https://storage.yandexcloud.net/yc-vpc-packer-export/monops/${var.monops_image_file}"
  labels        = {}
  os_type       = "LINUX"
  min_disk_size = 10
}

resource "yandex_container_registry" "monops_registry" {
  lifecycle {
    prevent_destroy = true
  }

  folder_id = var.folder_id
  name = "monops"
}

resource "yandex_vpc_security_group" "monops_web_sg" {
  folder_id = var.folder_id
  network_id = var.monops_network_id
  name = "monops-web-sg"
  description = "Security group for monops-web instances"

  ingress {
    protocol = "TCP"
    description = "Allow healthchecks"
    v4_cidr_blocks = ["198.18.235.0/24", "198.18.248.0/24"]
    v6_cidr_blocks = [var.hc_network_ipv6]
    port = var.monops_healthcheck_port
  }

  ingress {
    protocol = "TCP"
    description = "Allow SSH from Yandex"
    v6_cidr_blocks = ["2a02:6b8::/32"]
    port = 22
  }

  ingress {
    protocol = "TCP"
    description = "Allow web access from Yandex"
    v6_cidr_blocks = ["2a02:6b8::/32"]
    port = var.monops_web_port
  }

  ingress {
    protocol = "ICMP"
    description = "Allow pings"
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
    port = "0"
  }

  ingress {
    protocol = "ANY"
    description = "Allow inter-group communication"
    predefined_target = "self_security_group"
    from_port = 0
    to_port = 65535
  }

  egress {
    protocol = "ANY"
    description = "Allow any egress traffic"
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
    from_port = 0
    to_port = 65535
  }
}

data "external" "skm_metadata" {
  program = [
    "python3", "${path.module}/../../../shared/skm/generate.py"
  ]
  query = {
    secrets_file = "${abspath(path.module)}/secrets.yaml",

    cache_dir = "${path.root}/.skm-cache/monops/",
    ycp_profile = var.ycp_profile,
    yc_endpoint = var.yc_endpoint,
    key_uri = "yc-kms://${yandex_kms_symmetric_key.monops_kms_key.id}"

    yav_viewer_sa_secrets = var.yav_viewer_sa_secrets
    pem_file_name = "${var.monops_host}.pem"
  }
}

locals {
  monops_mongo_hosts = join(",", [
    for host in yandex_mdb_mongodb_cluster.monops.host:
    host.name
  ])

  cloud_init = <<-EOF
    #cloud-config
    datasource:
      Ec2:
        strict_id: false

    write_files:
      - path: /etc/default/monops
        content: |
          MONOPS_CR_ENDPOINT=${var.cr_endpoint}
          MONOPS_WEB_IMAGE="${var.cr_endpoint}/${yandex_container_registry.monops_registry.id}/monops:latest"
          MONGO_HOSTS=${local.monops_mongo_hosts}
          MONGO_PORT=27018
          MONGO_SSL_ENABLED=true

      - path: /etc/yc/monops/auth.yaml
        content: |
          oauthTokenEndpoint: ${var.oauth_endpoint}
          sessionEndpoint: ${var.ss_endpoint}
          iamEndpoint: ${var.iam_endpoint}
          authClient:
            host: ${var.as_host}
            port: 4286

          userFolder: ${var.folder_id}
          authClientId: yc.oauth.monops
          redirectBack: https://${var.monops_host}/auth
          federationId: yc.yandex-team.federation
          sessionHost: ${var.monops_host}
          bypassEnvironments: ${jsonencode(var.auth_environments_bypass)}
          proxyEnvironments: ${jsonencode(var.auth_environments_proxy)}
  EOF
}

resource "ycp_microcosm_instance_group_instance_group" "monops_web_ig" {
  folder_id = var.folder_id

  name = "monops-web"
  description = "Group of backends for monops-web"
  service_account_id = ycp_iam_service_account.monops_deploy_sa.id

  instance_template {
    name = "monops-web-{instance.internal_dc}{instance.index_in_zone}"
    hostname = "{instance.index_in_zone}.{instance.internal_dc}.web.${var.dns_zone}"
    fqdn = "{instance.index_in_zone}.{instance.internal_dc}.web.${var.dns_zone}"
    service_account_id = ycp_iam_service_account.monops_web_sa.id

    labels = {
      layer = "iaas"
      abc_svc = "ycvpc"
      env = var.environment
    }

    boot_disk {
      disk_spec {
        image_id = ycp_compute_image.monops_image.id
        size = 60
      }
    }

    platform_id = "standard-v2"
    resources {
      memory = 4
      cores = 2
      core_fraction = 20
    }

    // NOTE: required to avoid permanent diff
    scheduling_policy {
      deny_deallocation     = false
      preemptible           = false
      start_without_compute = false
    }

    network_interface {
      network_id = var.monops_network_id
      subnet_ids = values(var.monops_network_subnet_ids)
      security_group_ids = [yandex_vpc_security_group.monops_web_sg.id]

      primary_v4_address {
        name = "ipv4"
      }
      primary_v6_address {
        name = "ipv6"
        dns_record_spec {
          fqdn = "{instance.index_in_zone}.{instance.internal_dc}.web.${var.dns_zone}."
          dns_zone_id = var.dns_zone_id
          ptr = true
        }
        dns_record_spec {
          fqdn = "{instance.internal_dc}.web.${var.dns_zone}."
          dns_zone_id = var.dns_zone_id
        }
        dns_record_spec {
          fqdn = "web.${var.dns_zone}."
          dns_zone_id = var.dns_zone_id
        }
      }
    }

    metadata = {
      user-data = local.cloud_init
      enable-oslogin = "true"
      skm = data.external.skm_metadata.result.skm
    }
  }

  allocation_policy {
    dynamic zone {
      for_each = var.monops_zones
      content {
        zone_id = zone.value
      }
    }
  }

  scale_policy {
    fixed_scale {
      size = length(var.monops_zones)
    }
  }

  deploy_policy {
    max_unavailable = 2
    max_creating = 3
    max_expansion = 1
    max_deleting = 1
  }

  load_balancer_spec {
    target_group_spec {
      name = "monops-web-tg"
      address_names = ["ipv6"]
    }
  }

  health_checks_spec {
    health_check_spec {
      // NOTE: Restarting docker might require re-pulling layers
      // It is quite slow, so set unhealthy_threshold to 3 mins
      interval            = "24s"
      timeout             = "2s"
      healthy_threshold   = 2
      unhealthy_threshold = 8
      http_options {
        path = "/ping"
        port = 8440
      }
    }
  }
}

