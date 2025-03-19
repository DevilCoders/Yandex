resource "yandex_compute_image" "api" {
  folder_id = var.folder_id

  name = "api"
  description = "Image for API service built with packer from Mr.Prober images collection"
  source_url = "https://storage.yandexcloud.net/yc-vpc-packer-export/mr-prober/api/fd8g83f894nl3kiun82m.qcow2"

  timeouts {
    create = "10m"
  }
}

resource "yandex_vpc_security_group" "api" {
  folder_id = var.folder_id

  network_id = ycp_vpc_network.mr_prober_control.id
  name = "api-sg"
  description = "Security group for API"

  ingress {
    protocol = "TCP"
    description = "Allow SSH"
    v6_cidr_blocks = ["2a02:6b8::/32"]
    port = 22
  }

  ingress {
    protocol = "ICMP"
    description = "Allow pings"
    v4_cidr_blocks = ["0.0.0.0/0"]
    port = "0"
  }

  ingress {
    protocol = "IPV6_ICMP"
    description = "Allow pings"
    v6_cidr_blocks = ["::/0"]
    port = "0"
  }

  dynamic "ingress" {
    for_each = toset([80, 81])
    content {
      protocol = "TCP"
      description = "Allow ${ingress.key}/tcp from balancer nodes and network"
      v4_cidr_blocks = concat(
        ["198.18.235.0/24", "198.18.248.0/24"],
        values(var.control_network_ipv4_cidrs)
      )
      v6_cidr_blocks = concat(
        [var.hc_network_ipv6, "2a02:6b8::/32"],
        values(var.control_network_ipv6_cidrs)
      )
      port = ingress.key
    }
  }

  dynamic "ingress" {
    for_each = toset([8080, 16300, 22132])
    content {
      protocol = "TCP"
      description = "Allow ${ingress.key}/tcp from yandex: it's solomon-agent/unified-agent port"
      v6_cidr_blocks = ["2a02:6b8::/32", "2a11:f740::/32"]
      port = ingress.key
    }
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

data "external" "api_skm_metadata" {
  program = [
    "python3", "${path.module}/../../shared/skm/generate.py"
  ]
  query = {
    secrets_file = "${abspath(path.module)}/api_secrets.yaml",

    cache_dir = "${path.root}/.skm-cache/mr-prober-${var.mr_prober_environment}/",
    ycp_profile = var.ycp_profile,
    yc_endpoint = var.yc_endpoint,
    mr_prober_environment = var.mr_prober_environment,
    s3_stand = var.s3_stand,
    key_uri = "yc-kms://${yandex_kms_symmetric_key.mr_prober_secret_kek.id}"
  }
}

resource "ycp_microcosm_instance_group_instance_group" "api" {
  folder_id = var.folder_id

  name = "api"
  description = "Group of API backends"

  service_account_id = yandex_iam_service_account.mr_prober_sa.id

  instance_template {
    boot_disk {
      disk_spec {
        type_id = "network-ssd"
        image_id = yandex_compute_image.api.id
        size = 50
      }
    }

    resources {
      memory = var.api_vm_memory
      cores = var.api_vm_cores
      core_fraction = var.api_vm_fraction
    }

    network_interface {
      network_id = ycp_vpc_network.mr_prober_control.id
      subnet_ids = values(local.control_network_subnet_ids)
      security_group_ids = [yandex_vpc_security_group.api.id]
      primary_v4_address {}
      primary_v6_address {
        name = "v6"
        dns_record_spec {
          fqdn = "{instance.index_in_zone}.{instance.internal_dc}.api.${var.dns_zone}."
          dns_zone_id = ycp_dns_dns_zone.mr_prober.id
          ptr = true
        }
      }
    }

    name = "api-{instance.internal_dc}{instance.index_in_zone}"
    description = "API backend in {instance.internal_dc}"
    hostname = "{instance.index_in_zone}.{instance.internal_dc}.api.${var.dns_zone}"
    fqdn = "{instance.index_in_zone}.{instance.internal_dc}.api.${var.dns_zone}"

    metadata = {
      user-data = templatefile(
        "${path.module}/cloud-init.yaml",
        {
          hostname = "{instance.index_in_zone}.{instance.internal_dc}.api.${var.dns_zone}",
          s3_endpoint = var.s3_endpoint,
          stand_name = var.mr_prober_environment,
          // It's needed to be here because we share cloud-init.yaml between API and Creator. Maybe we should split them?
          grpc_iam_api_endpoint = var.grpc_iam_api_endpoint,
          grpc_compute_api_endpoint = var.grpc_compute_api_endpoint,
          api_domain = var.api_domain,
          meeseeks_compute_node_prefixes_cli_param = var.meeseeks_compute_node_prefixes_cli_param
        }
      )
      enable-oslogin = "true"
      skm = data.external.api_skm_metadata.result.skm
    }

    labels = {
      layer = "iaas"
      abc_svc = "ycvpc"
      env = var.environment
    }

    platform_id = var.platform_id

    service_account_id = yandex_iam_service_account.mr_prober_sa.id
  }

  allocation_policy {
    dynamic zone {
      for_each = var.api_zones
      content {
        zone_id = zone.value
      }
    }
  }

  deploy_policy {
    max_unavailable = 2
    max_creating = 3
    max_expansion = 1
    max_deleting = 1
  }

  scale_policy {
    fixed_scale {
      size = 3
    }
  }

  application_load_balancer_spec {
    target_group_spec {
      name = "apis-target-group"
      description = "Target group for Application Load balancer for API"
      address_names = ["v6"]
    }
  }

  ### Now HCaaS is incompatible with ALB,
  ### if healthchecks are running on the same port as ALB, so we use additional port 81

  health_checks_spec {
    health_check_spec {
      address_names = ["v6"]
      interval = "20s"
      timeout = "5s"
      healthy_threshold = 2
      unhealthy_threshold = 6
      http_options {
        port = 81
        path = "/health"
      }
    }
  }

  depends_on = [
    ycp_resource_manager_folder_iam_member.mr_prober_sa
  ]
}

# Application Load Balancer

resource "ycp_vpc_address" "api_alb_address" {
  lifecycle {
    prevent_destroy = true
  }

  name = "mr-prober-api-application-load-balancer-address"
  folder_id = var.folder_id

  ipv6_address_spec {
    requirements {
      hints = ["yandex-only"]
    }
  }

  reserved = true
}

resource "yandex_dns_recordset" "api_balancer" {
  lifecycle {
    prevent_destroy = true
  }

  name = "${var.api_domain}."
  zone_id = ycp_dns_dns_zone.mr_prober.id
  type = "AAAA"
  ttl = 200
  data = [
    ycp_vpc_address.api_alb_address.ipv6_address[0].address
  ]
}

# See https://cloud.yandex.ru/docs/application-load-balancer/concepts/application-load-balancer#security-groups
resource "yandex_vpc_security_group" "api_alb" {
  folder_id = var.folder_id

  network_id = ycp_vpc_network.mr_prober_control.id
  name = "api-alb-sg"
  description = "Security group for API application load balancer"

  ingress {
    protocol = "ICMP"
    description = "Allow pings"
    v4_cidr_blocks = ["0.0.0.0/0"]
    port = "0"
  }

  ingress {
    protocol = "IPV6_ICMP"
    description = "Allow pings"
    v6_cidr_blocks = ["::/0"]
    port = "0"
  }

  ingress {
    protocol = "TCP"
    description = "Allow 30080/tcp from balancer nodes"
    v4_cidr_blocks = concat(
      ["198.18.235.0/24", "198.18.248.0/24"]
    )
    v6_cidr_blocks = concat(
      [var.hc_network_ipv6, "2a02:6b8::/32"]
    )
    port = 30080
  }

  ingress {
    protocol = "ANY"
    description = "Allow any ingress traffic from the internet to 80/tcp"
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
    port = 80
  }

  ingress {
    protocol = "ANY"
    description = "Allow any ingress traffic from the internet to 443/tcp"
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
    port = 443
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

# See https://wiki.yandex-team.ru/cloud/devel/platform-team/l7/usage/

resource "ycp_platform_alb_load_balancer" "api" {
  name = "mr-prober-api-alb"
  folder_id = var.folder_id

  listener {
    name = "http"

    endpoint {
      ports = [80]

      address {
        external_ipv6_address {
          address = ycp_vpc_address.api_alb_address.ipv6_address[0].address
          yandex_only = true
        }
      }
    }

    http {
      redirects {
        http_to_https = true
      }
    }
  }

  listener {
    name = "tls"

    endpoint {
      ports = [443]

      address {
        external_ipv6_address {
          address = ycp_vpc_address.api_alb_address.ipv6_address[0].address
          yandex_only = true
        }
      }
    }

    tls {
      default_handler {
        certificate_ids = [ycp_certificatemanager_certificate_request.api.id]

        http_handler {
          allow_http10   = false
          http_router_id = ycp_platform_alb_http_router.api.id
          is_edge = false

          http2_options {
            initial_stream_window_size     = 1024 * 1024
            initial_connection_window_size = 1024 * 1024
            max_concurrent_streams = 100
          }
        }
      }
      tcp_options {
        connection_buffer_limit_bytes = 32768
      }
    }
  }

  region_id = var.api_alb_region
  network_id = ycp_vpc_network.mr_prober_control.id

  allocation_policy {
    dynamic location {
      for_each = {for subnet in ycp_vpc_subnet.mr_prober_control: subnet.zone_id => subnet.id}
      content {
        subnet_id = location.value
        zone_id   = location.key

        disable_traffic = false
      }
    }
  }

  security_group_ids = [
    yandex_vpc_security_group.api_alb.id
  ]

  # Enable Jaeger traces
  tracing {
    # Preprod: https://jaeger.private-api.ycp.cloud-preprod.yandex.net
    # Prod: https://jaeger.private-api.ycp.cloud.yandex.net
    service_name = "mr-prober-api-${var.mr_prober_environment}"
  }

  # Cluster for metrics in the Solomon
  # Preprod: https://solomon.cloud-preprod.yandex-team.ru/?project=aoem2v5as6lv1ebgg1cu
  # Prod: https://solomon.cloud.yandex-team.ru/?project=b1grffd2lfm69s7koc4t
  solomon_cluster_name = "mr-prober-api-${var.mr_prober_environment}"

  # Host for alerts in the Juggler
  juggler_host = var.api_domain

  depends_on = [
    ycp_microcosm_instance_group_instance_group.api,
  ]
}

resource "ycp_platform_alb_http_router" "api" {
  name = "mr-prober-api-alb-http-router"
  folder_id = var.folder_id
}

resource "ycp_platform_alb_virtual_host" "api" {
  name = "mr-prober-api-alb-virtual-host"

  authority = [var.api_domain]
  http_router_id = ycp_platform_alb_http_router.api.id

  route {
    name = "default"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = ycp_platform_alb_backend_group.api.id
      }
    }
  }
}

resource "ycp_platform_alb_backend_group" "api" {
  name = "mr-prober-api-alb-backend-group"
  folder_id = var.folder_id

  http {
    backend {
      name = "api"

      port = 80
      backend_weight = 1

      target_group {
        target_group_id = ycp_microcosm_instance_group_instance_group.api.application_load_balancer_state[0].target_group_id
      }

      healthchecks {
        timeout             = "5s"
        interval            = "20s"
        healthy_threshold   = 2
        unhealthy_threshold = 1
        http {
          path = "/health"
        }
      }
    }
  }
}
