locals {
  http_router_id = "ds78ms0obd2ppvg56vjk"
}

resource "ycp_platform_alb_backend_group" "alb_collector_backend_group" {
  name        = "jaeger-collector-direct-bg"
  description = "Jaeger Collector Direct Backend Group"
  grpc {
    backend {
      name   = "jaeger-collector-direct-backend"
      weight = -1
      port   = 14250
      target_group {
        target_group_id = ycp_microcosm_instance_group_instance_group.collector_direct_ig.platform_l7_load_balancer_state[0].target_group_id
      }
      healthchecks {
        timeout             = "0.200s"
        interval            = "1s"
        healthy_threshold   = 2
        unhealthy_threshold = 3
        healthcheck_port    = 15000
        http {
          path = "/ping"
        }
      }
    }
    backend {
      name   = "jaeger-collector-lb-backend"
      weight = 100
      port   = 14250
      target_group {
        target_group_id = ycp_microcosm_instance_group_instance_group.collector_lb_ig.platform_l7_load_balancer_state[0].target_group_id
      }
      healthchecks {
        timeout             = "0.200s"
        interval            = "1s"
        healthy_threshold   = 2
        unhealthy_threshold = 3
        healthcheck_port    = 16000
        http {
          path = "/ping"
        }
      }
    }
    connection {}
  }
}

resource "ycp_platform_alb_virtual_host" "vh-jaeger-query-ydb-private-api-prod" {
  authority = [
    "jaeger-ydb.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = local.http_router_id
  name           = "vh-jaeger-query-ydb-private-api-prod"
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
        auto_host_rewrite  = false
        backend_group_id   = ycp_platform_alb_backend_group.alb_query_ydb2_backend_group.id
        support_websockets = false
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-jaeger-query-yt-private-api-prod" {
  authority = [
    "jaeger.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = local.http_router_id
  name           = "vh-jaeger-query-yt-private-api-prod"
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
        auto_host_rewrite  = false
        backend_group_id   = ycp_platform_alb_backend_group.alb_query_ydb2_backend_group.id
        support_websockets = false
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh_jaeger_collector_private_api_prod" {
  authority = [
    "jaeger-collector.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = local.http_router_id
  name           = "vh-jaeger-collector-private-api-prod"
  ports = [
    443,
  ]
  route {
    name = "main_route"
    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }
      route {
        auto_host_rewrite = false
        backend_group_id  = ycp_platform_alb_backend_group.alb_collector_backend_group.id
      }
    }
  }
}
