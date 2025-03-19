resource ycp_load_balancer_target_group ylb {
  folder_id   = local.folder_id
  region_id   = "ru-central1"
  labels      = {}
  description = "tg for ylb load test"
  target {
    address   = ycp_compute_instance.overlay_vms["target"].network_interface[0].primary_v6_address[0].address
    subnet_id = local.subnet_ids["ru-central1-a"].id
  }
}

resource ycp_load_balancer_network_load_balancer ylb {
  attached_target_group {
    health_check {
      name                = "healthcheck"
      timeout             = "1s"
      interval            = "2s"
      healthy_threshold   = 3
      unhealthy_threshold = 2
      http_options {
        path = "/ping"
        port = 30080
      }
    }
    target_group_id = ycp_load_balancer_target_group.ylb.id
  }
  type = "EXTERNAL"
  listener_spec {
    name = "yandex-only-listener"
    external_address_spec {
      address     = "2a0d:d6c0:0:ff1a::b4"
      ip_version  = "IPV6"
      yandex_only = true
    }
    port        = 80
    target_port = 30080
    protocol    = "TCP"
  }
  folder_id   = local.folder_id
  description = "tg for ylb load test"
  region_id   = "ru-central1"
}

resource "ycp_dns_dns_record_set" ylb {
  zone_id = local.dns_zone_id
  name    = "load-selfhost-ylb"
  type    = "AAAA"
  ttl     = "3600"
  data    = [ycp_load_balancer_network_load_balancer.ylb.listener_spec[0].external_address_spec[0].address]
}
