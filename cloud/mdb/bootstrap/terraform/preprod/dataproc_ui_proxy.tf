resource "yandex_iam_service_account" "dataproc-ui-proxy" {
  name = "dataproc-ui-proxy"
}

resource "ycp_vpc_network" "dataproc-ui-proxy" {
  name = "cloud-dataproc-ui-proxy-preprod-nets"
}

resource "ycp_vpc_subnet" "dataproc-ui-proxy-rc1a" {
  name           = "cloud-dataproc-ui-proxy-preprod-nets-ru-central1-a"
  network_id     = ycp_vpc_network.dataproc-ui-proxy.id
  v6_cidr_blocks = ["2a02:6b8:c0e:501:0:fc27::/96"]
}

resource "ycp_vpc_subnet" "dataproc-ui-proxy-rc1b" {
  name           = "cloud-dataproc-ui-proxy-preprod-nets-ru-central1-b"
  network_id     = ycp_vpc_network.dataproc-ui-proxy.id
  v6_cidr_blocks = ["2a02:6b8:c02:901:0:fc27::/96"]
}

resource "ycp_vpc_subnet" "dataproc-ui-proxy-rc1c" {
  name           = "cloud-dataproc-ui-proxy-preprod-nets-ru-central1-c"
  network_id     = ycp_vpc_network.dataproc-ui-proxy.id
  v6_cidr_blocks = ["2a02:6b8:c03:501:0:fc27::/96"]
}

resource "ycp_compute_instance" "dataproc-ui-proxy-preprod01k" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "dataproc-ui-proxy-preprod01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v2"
  fqdn                      = "dataproc-ui-proxy-preprod01k.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 2
  }

  boot_disk {
    disk_spec {
      size     = 32
      image_id = "fdvn6fk0cgjrg3405rhj" # yc compute image get-latest-from-family common-1if --profile mdb-preprod-cp
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
    ip_address = ycp_compute_instance.dataproc-ui-proxy-preprod01k.network_interface.0.primary_v6_address.0.address
    subnet_id  = ycp_compute_instance.dataproc-ui-proxy-preprod01k.network_interface.0.subnet_id
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
        sni = "dataproc-ui-proxy.private-api.cloud-preprod.yandex.net"
      }
    }

    connection {}
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
