provider "ycp" {
  prod      = true
  folder_id = "${var.yc_folder}"
  zone      = "${var.yc_zone}"
  ycp_profile = "trail-prod"
}

resource "ycp_platform_alb_target_group" "trail_target_group" {
  name        = "trail-target-group-prod"
  description = "Target Group for Cloud Trail ALB (prod)"
  folder_id   = var.yc_folder

  labels      = {
    layer   = "paas"
    abc_svc = "yccloudtrail"
    env     = "prod"
  }

  dynamic "target" {
    for_each = "${var.subnets_addresses}"

    content {
      ip_address = target.value
      subnet_id  = target.key
    }
  }
}

resource "ycp_platform_alb_backend_group" "trail_backend_group" {
  name        = "trail-backend-group-prod"
  description = "Backend Group for Cloud Trail (prod)"
  folder_id   = var.yc_folder
  labels      = {
    layer   = "paas"
    abc_svc = "yccloudtrail"
    env     = "prod"
  }

  grpc {
    backend {
      name   = "trail-api-backend-prod"
      port   = 443
      weight = 100

      healthchecks {
        interval            = "1s"
        timeout             = "0.200s"
        healthy_threshold   = 2
        unhealthy_threshold = 3
        healthcheck_port    = 9982

        http {
          path = "/ping"
        }
      }

      passive_healthcheck {
        max_ejection_percent                  = 66
        consecutive_gateway_failure           = 2
        base_ejection_time                    = "30s"
        enforcing_consecutive_gateway_failure = 100
        interval                              = "10s"
      }

      target_group {
        target_group_id = ycp_platform_alb_target_group.trail_target_group.id
      }

      tls {}
    }
  }
}
