resource "ycp_compute_instance" "dataproc-ui-proxy01k" {
  name        = "dataproc-ui-proxy01k"
  zone_id     = local.zones.zone_a.id
  platform_id = "standard-v2"
  fqdn        = "dataproc-ui-proxy01k.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 50
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = 32
      image_id = "fd86hn54t1n03lngfeba" # yc compute image get-latest-from-family common-1if --profile mdb-prod-cp
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dataproc-ui-proxy-rc1a.id
    primary_v6_address {}
  }
}

resource "ycp_platform_alb_target_group" "dataproc-ui-proxy" {
  description = "ALB target group for dataproc-ui-proxy"
  folder_id   = var.folder_id
  labels      = {}
  name        = "dataproc-ui-proxy"

  # Service backend addresses
  target {
    ip_address = ycp_compute_instance.dataproc-ui-proxy01k.network_interface.0.primary_v6_address.0.address
    subnet_id  = ycp_compute_instance.dataproc-ui-proxy01k.network_interface.0.subnet_id
  }
}

resource "ycp_platform_alb_backend_group" "dataproc-ui-proxy" {
  description = "ALB backend group for dataproc-ui-proxy"
  folder_id   = var.folder_id
  labels      = {}
  name        = "dataproc-ui-proxy"

  http {
    backend {
      name      = "http-backend"
      port      = 443
      weight    = 100
      use_http2 = true

      target_group {
        # Use target group resource created above.
        target_group_id = ycp_platform_alb_target_group.dataproc-ui-proxy.id
      }

      tls {
        sni = "dataproc-ui-proxy.private-api.yandexcloud.net"
      }
    }

    connection {}
  }
}

resource "ycp_platform_alb_http_router" "dataproc-ui-proxy" {
  name = "dataproc-ui-proxy"
}

resource "ycp_platform_alb_virtual_host" "vh-dataproc-ui" {
  authority = [
    "*.dataproc-ui.yandexcloud.net",
    "dataproc-ui.yandexcloud.net",
  ]
  http_router_id = ycp_platform_alb_http_router.dataproc-ui-proxy.id
  name           = "vh-dataproc-ui"
  ports = [
    443,
  ]

  modify_request_headers {
    name    = "X-Forwarded-Host"
    remove  = false
    replace = "%REQ(:authority)%"
  }

  route {
    name = "main_route"

    http {
      match {
        http_method = []

        path {
          prefix_match = "/"
        }
      }

      route {
        backend_group_id = ycp_platform_alb_backend_group.dataproc-ui-proxy.id
      }
    }
  }
}

resource "yandex_vpc_security_group" "dataproc-ui-proxy-sg" {
  name        = "dataproc-ui-proxy-sg"
  description = "Dataproc UI proxy security group"
  network_id  = ycp_vpc_network.dataproc-ui-proxy.id

  # Ingress rules
  ingress {
    protocol       = "TCP"
    description    = "Allow incoming traffic to SSH"
    port           = 22
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    protocol       = "TCP"
    description    = "Ingress HTTP/GRPC"
    port           = 443
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    protocol       = "TCP"
    description    = "Ingress HTTP 80"
    port           = 80
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    protocol       = "UDP"
    description    = "Ingress DNS UDP"
    port           = 53
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    protocol          = "ANY"
    description       = "Self INGRESS"
    from_port         = 0
    to_port           = 65535
    predefined_target = "self_security_group"
  }

  ingress {
    protocol       = "IPV6_ICMP"
    description    = "Ingress ICMP"
    v6_cidr_blocks = local.v6-yandex-and-cloud
    from_port      = 0
    to_port        = 32767
  }

  # Egress rules
  # Allow ICMP IPv6, TCP, UDP on non-ephemeral ports.
  egress {
    protocol       = "IPV6_ICMP"
    description    = "Egress ICMP"
    v6_cidr_blocks = local.v6-yandex-and-cloud
    from_port      = 0
    to_port        = 32767
  }

  egress {
    protocol       = "TCP"
    description    = "Egress TCP"
    v6_cidr_blocks = local.v6-yandex-and-cloud
    from_port      = 0
    to_port        = 32767
  }

  egress {
    protocol       = "UDP"
    description    = "Egress UDP"
    v6_cidr_blocks = local.v6-yandex-and-cloud
    from_port      = 0
    to_port        = 32767
  }

  egress {
    protocol          = "ANY"
    description       = "Self EGRESS"
    from_port         = 0
    to_port           = 65535
    predefined_target = "self_security_group"
  }
}
