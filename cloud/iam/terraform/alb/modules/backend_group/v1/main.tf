locals {
  backend_name = var.backend_name != null ? var.backend_name : var.name
}

resource "ycp_platform_alb_backend_group" "backend_group" {
  name        = var.name
  description = var.description == null ? format("%s at %s", var.name, var.environment) : var.description
  folder_id   = var.folder_id
  labels      = {}

  dynamic grpc {
    for_each = var.is_http ? [] : [1]
    content {
      backend {
        name           = local.backend_name
        port           = var.backend_port
        backend_weight = var.backend_weight

        healthchecks {
          healthy_threshold   = var.healthcheck_healthy_threshold
          unhealthy_threshold = var.healthcheck_unhealthy_threshold
          interval            = var.healthcheck_interval
          timeout             = var.healthcheck_timeout
          healthcheck_port    = var.healthcheck_port

          http {
            path = var.http_healthcheck_path
          }
        }

        target_group {
          target_group_id = var.target_group_id
        }

        tls {
        }
      }
    }
  }

  dynamic http {
    for_each = var.is_http ? [1] : []
    content {
      backend {
        name           = local.backend_name
        port           = var.backend_port
        backend_weight = var.backend_weight

        healthchecks {
          healthy_threshold   = var.healthcheck_healthy_threshold
          unhealthy_threshold = var.healthcheck_unhealthy_threshold
          interval            = var.healthcheck_interval
          timeout             = var.healthcheck_timeout
          healthcheck_port    = var.healthcheck_port

          http {
            path = var.http_healthcheck_path
          }
        }

        target_group {
          target_group_id = var.target_group_id
        }

        tls {
        }
      }
    }
  }
}
