module "iam_service_backend_groups_prod" {
  source = "../modules/backend_group/v1"

  for_each = local.iam_service_settings_prod

  is_http               = each.value.is_http
  name                  = format("%s-backend", each.value.name)
  backend_name          = lookup(each.value, "backend_name", null)
  description           = format("%s-backend at prod", each.value.name)
  backend_port          = each.value.backend_port
  healthcheck_port      = each.value.healthcheck_port
  http_healthcheck_path = each.value.http_healthcheck_path
  backend_weight        = lookup(each.value, "backend_weight", 100)

  # sni = "oauth.private-api.cloud.yandex.net"

  folder_id       = "yc.iam.service-folder" # TODO local.openid_folder.id
  target_group_id = ycp_platform_alb_target_group.iam_openid_target_group.id
  environment     = "prod"
}

resource "ycp_platform_alb_backend_group" iam_cp_server_backend {
  # id        = "ds79kapt4s13ul7250co"
  name        = "iam-cp-server-backend"
  description = "Backend for IAM Control Plane Server"
  folder_id   = "yc.iam.control-plane-folder"
  labels      = {}

  grpc {
    backend {
      name           = "iam-cp-server-backend"
      backend_weight = 1

      target {
        endpoint {
          hostname = "iam.private-api.cloud.yandex.net"
          port     = 4283
        }
      }

      tls {
        sni = "iam.private-api.cloud.yandex.net"
      }
    }
  }
}

resource "ycp_platform_alb_backend_group" rm_cp_server_backend {
  # id        = "ds7jipv3qrqghhlm13ea"
  name        = "rm-cp-server-backend"
  description = "Backend for ResourceManager Control Plane Server"
  folder_id   = "yc.iam.rm-control-plane-folder"
  labels      = {}

  grpc {
    backend {
      name           = "rm-cp-server-backend"
      backend_weight = 1

      target {
        endpoint {
          hostname = "rm.private-api.cloud.yandex.net"
          port     = 4284
        }
      }

      tls {
        sni = "rm.private-api.cloud.yandex.net"
      }
    }
  }
}

resource "ycp_platform_alb_backend_group" ts_server_backend {
  # id        = "ds7eitk0omhsub8cgfu0"
  name = "token-service-backend"
  description = "Backend for Token Service"
  folder_id = "yc.iam.token-service-folder"
  labels = {}

  grpc {
    backend {
      name = "token-service-backend"
      backend_weight = 1

      target {
        endpoint {
          hostname = "ts.private-api.cloud.yandex.net"
          port = 4282
        }
      }

      tls {
        sni = "ts.private-api.cloud.yandex.net"
      }
    }
  }
}
