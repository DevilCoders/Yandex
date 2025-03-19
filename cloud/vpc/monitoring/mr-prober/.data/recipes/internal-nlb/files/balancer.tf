resource "yandex_lb_target_group" "balancer_v4" {
  folder_id = var.folder_id
  name = "${var.prefix}-backends-target-group-v4"
  region_id = "ru-central1"

  dynamic "target" {
    for_each = local.backend_instances
    content {
      subnet_id = target.value.network_interface.0.subnet_id
      address = target.value.network_interface.0.ip_address
    }
  }
}

resource "yandex_lb_target_group" "balancer_v6" {
  folder_id = var.folder_id
  name = "${var.prefix}-backends-target-group-v6"
  region_id = "ru-central1"

  dynamic "target" {
    for_each = local.backend_instances
    content {
      subnet_id = target.value.network_interface.0.subnet_id
      address = target.value.network_interface.0.ipv6_address
    }
  }
}

resource "yandex_lb_network_load_balancer" "balancer_v4" {
  folder_id = var.folder_id

  name = "${var.prefix}-load-balancer-ipv4"
  description = "Internal IPv4 load balancer for backends"

  type = "internal"

  listener {
    name = "${var.prefix}-listener-ipv4"
    port = var.target_port
    internal_address_spec {
      subnet_id = values(ycp_vpc_subnet.subnets)[0].id
      ip_version = "ipv4"
    }
  }

  attached_target_group {
    target_group_id = yandex_lb_target_group.balancer_v4.id

    healthcheck {
      name = "http"
      interval = 10
      timeout = 5
      healthy_threshold = 2
      unhealthy_threshold = 3
      http_options {
        port = var.target_port
        path = "/"
      }
    }
  }
}

resource "yandex_lb_network_load_balancer" "balancer_v6" {
  folder_id = var.folder_id

  name = "${var.prefix}-load-balancer-ipv6"
  description = "IPv6 load balancer for backends"

  type = "external"

  listener {
    name = "${var.prefix}-listener-ipv6"
    port = 80
    external_address_spec {
      ip_version = "ipv6"
    }
  }

  attached_target_group {
    target_group_id = yandex_lb_target_group.balancer_v6.id

    healthcheck {
      name = "http"
      interval = 10
      timeout = 5
      healthy_threshold = 2
      unhealthy_threshold = 3
      http_options {
        port = 80
        path = "/"
      }
    }
  }
}

resource "yandex_dns_recordset" "balancer_v4" {
  lifecycle {
    prevent_destroy = true
  }

  name = "ipv4.${var.prefix}.${var.dns_zone}."
  zone_id = var.dns_zone_id
  type = "A"
  ttl = 200
  data = [
    for listener in yandex_lb_network_load_balancer.balancer_v4.listener:
    tolist(listener.internal_address_spec).0.address if listener.name == "${var.prefix}-listener-ipv4"
  ]
}

resource "yandex_dns_recordset" "balancer_v6" {
  lifecycle {
    prevent_destroy = true
  }

  name = "ipv6.${var.prefix}.${var.dns_zone}."
  zone_id = var.dns_zone_id
  type = "AAAA"
  ttl = 200
  data = [
    for listener in yandex_lb_network_load_balancer.balancer_v6.listener:
    tolist(listener.external_address_spec).0.address if listener.name == "${var.prefix}-listener-ipv6"
  ]
}
