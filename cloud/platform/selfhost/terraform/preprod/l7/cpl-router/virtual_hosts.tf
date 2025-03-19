locals {
  api_adapter_internal_bg = "a5dce5a6lkipt6u2op8m"

  vpc_grpc_api_internal_bg     = "a5dea6t3hsotkqv093sl" # https://st.yandex-team.ru/CLOUD-45063
  hcaas_grpc_api_internal_bg   = "a5d0r6k6e8mv95r6eohc" # https://st.yandex-team.ru/CLOUD-62714
  compute_grpc_api_internal_bg = "a5df504vjjcb52figk7i" # https://st.yandex-team.ru/CLOUD-47667
  iam_cp_grpc_api_internal_bg  = "a5d9i7dihhinn97fea62" # https://st.yandex-team.ru/CLOUD-57270
  rm_cp_grpc_api_internal_bg   = "a5dkaf13arfap6ieaotm" # https://st.yandex-team.ru/CLOUD-57270
  ts_grpc_api_internal_bg      = "a5d39r1tgk2d4vnre4an" # https://st.yandex-team.ru/CLOUD-100899
}

# ycp_platform_alb_virtual_host.dns--preprod:
resource "ycp_platform_alb_virtual_host" "dns--preprod" {
  authority = [
    "dns.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "dns--preprod"
  ports = [
    443,
  ]

  route {
    name = "proxy_route"

    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "a5d66fmaeejoe2301tvq"
        idle_timeout      = "180s"
        max_timeout       = "180s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.dns-service-preprod:
# deprecated
resource "ycp_platform_alb_virtual_host" "dns-service-preprod" {
  authority = [
    "dns-service.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "dns-service-preprod"
  ports = [
    443,
  ]

  route {
    name = "proxy_route"

    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "a5d66fmaeejoe2301tvq"
        idle_timeout      = "60s"
        max_timeout       = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.tm-grpc-private-api-preprod:
resource "ycp_platform_alb_virtual_host" "tm-grpc-private-api-preprod" {
  authority = [
    "tm.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "tm-grpc-private-api-preprod"
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
        backend_group_id  = "a5dau8p7s14clqustj29"
        idle_timeout      = "60s"
        max_timeout       = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.tm-rest-private-api-preprod:
resource "ycp_platform_alb_virtual_host" "tm-rest-private-api-preprod" {
  authority = [
    "tm-rest.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "tm-rest-private-api-preprod"
  ports = [
    443,
  ]

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
        backend_group_id   = "a5d07bqrij7eji07p4js"
        idle_timeout       = "60s"
        support_websockets = false
        timeout            = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-api-adapter-preprod:
resource "ycp_platform_alb_virtual_host" "vh-api-adapter-preprod" {
  authority = [
    "api-adapter.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-api-adapter-preprod"
  ports = [
    443,
  ]

  # https://st.yandex-team.ru/CLOUD-47667
  # Route specific gRPC requests to Compute API directly.
  route {
    name = "compute-api"

    grpc {
      match {
        fqmn {
          prefix_match = "/yandex.cloud.priv.compute.v1."
        }
      }

      route {
        backend_group_id = local.compute_grpc_api_internal_bg
      }
    }
  }

  # https://st.yandex-team.ru/CLOUD-52282
  # Route all requests to services into VPC gRPC API backend.
  route {
    name = "vpc-api"

    grpc {
      match {
        fqmn {
          prefix_match = "/yandex.cloud.priv.vpc.v1."
        }
      }

      route {
        backend_group_id = local.vpc_grpc_api_internal_bg
      }
    }
  }

  # https://st.yandex-team.ru/CLOUD-70700
  # Route loadbalancer.v1. to VPC gRPC API backend.
  route {
    name = "loadbalancer-api"

    grpc {
      match {
        fqmn {
          prefix_match = "/yandex.cloud.priv.loadbalancer.v1."
        }
      }

      route {
        backend_group_id = local.vpc_grpc_api_internal_bg
      }
    }
  }

  # https://st.yandex-team.ru/CLOUD-62714
  # Route all requests to services into hcaas gRPC API backend.
  route {
    name = "hcaas"

    grpc {
      match {
        fqmn {
          prefix_match = "/yandex.cloud.priv.healthcheck.v1alpha."
        }
      }

      route {
        backend_group_id = local.hcaas_grpc_api_internal_bg
      }
    }
  }

  # https://st.yandex-team.ru/CLOUD-57270
  # Route all requests to services into IAM Control Plane gRPC API backend.
  #
  # y.c.p.iam.v1.IamTokenService/*                    -> token service
  # y.c.p.iam.v1.IamCookieService/*                   -> token service
  # y.c.p.iam.v1.AccessBindingService/*               -> api-adapter (not compatible with new API)
  # y.c.p.iam.v1.ServiceAccountService/IssueToken     -> api-adapter XXX https://st.yandex-team.ru/CLOUD-62030
  # y.c.p.iam.v1.ServiceAccountService/ListOperations -> api-adapter (merge of identity & iam-cp operations required)
  # y.c.p.iam.v1.OperationService/*                   -> api-adapter (merge of identity & iam-cp operations required)
  #
  # y.c.p.iam.v1.* -> IAM Control Plane gRPC API backend
  route {
    name = "iam-token-service"

    grpc {
      match {
        fqmn {
          prefix_match = "/yandex.cloud.priv.iam.v1.IamTokenService/"
        }
      }

      route {
        backend_group_id = local.ts_grpc_api_internal_bg
      }
    }
  }
  route {
    name = "iam-cookie-service"

    grpc {
      match {
        fqmn {
          prefix_match = "/yandex.cloud.priv.iam.v1.IamCookieService/"
        }
      }

      route {
        backend_group_id = local.ts_grpc_api_internal_bg
      }
    }
  }
  route {
    name = "iam-access-binding-service"

    grpc {
      match {
        fqmn {
          prefix_match = "/yandex.cloud.priv.iam.v1.AccessBindingService/"
        }
      }

      route {
        backend_group_id = local.api_adapter_internal_bg
      }
    }
  }
  route {
    name = "iam-sa-issue-token"

    grpc {
      match {
        fqmn {
          exact_match = "/yandex.cloud.priv.iam.v1.ServiceAccountService/IssueToken"
        }
      }

      route {
        backend_group_id = local.api_adapter_internal_bg
      }
    }
  }
  route {
    name = "iam-sa-list-operations"

    grpc {
      match {
        fqmn {
          exact_match = "/yandex.cloud.priv.iam.v1.ServiceAccountService/ListOperations"
        }
      }

      route {
        backend_group_id = local.api_adapter_internal_bg
      }
    }
  }
  route {
    name = "iam-operation-service"

    grpc {
      match {
        fqmn {
          prefix_match = "/yandex.cloud.priv.iam.v1.OperationService/"
        }
      }

      route {
        backend_group_id = local.api_adapter_internal_bg
      }
    }
  }
  # y.c.p.iam.v1.* -> IAM Control Plane gRPC API backend
  route {
    name = "iam-cp-api"

    grpc {
      match {
        fqmn {
          prefix_match = "/yandex.cloud.priv.iam.v1."
        }
      }

      route {
        backend_group_id = local.iam_cp_grpc_api_internal_bg
      }
    }
  }

  # resourcemanager.v1.* -> ResourceManager Control Plane gRPC API backend
  route {
    name = "rm-cp-api"

    grpc {
      match {
        fqmn {
          prefix_match = "/yandex.cloud.priv.resourcemanager.v1."
        }
      }

      route {
        backend_group_id = local.rm_cp_grpc_api_internal_bg
      }
    }
  }

  # Default route to api-adapter
  route {
    name = "main-route"

    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = local.api_adapter_internal_bg
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-container-registry-preprod:
resource "ycp_platform_alb_virtual_host" "vh-container-registry-preprod" {
  authority = [
    "container-registry.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-container-registry-preprod"
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
        backend_group_id  = "a5de6bmslcn1oa8q4spb"

        retry_policy {
          interval    = "1s"
          num_retries = 2
          retry_on = [
            "INTERNAL",
            "CANCELLED",
            "UNAVAILABLE",
            "RESOURCE_EXHAUSTED",
            "DEADLINE_EXCEEDED",
          ]
        }
      }
    }
  }
}

# ycp_platform_alb_virtual_host.vh-vs-controller-preprod:
resource "ycp_platform_alb_virtual_host" "vh-vulnerability-scanner-controller-preprod" {
  authority = [
    "vs-controller.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = "a5d39v77dpdjqsr001m4"
  name           = "vh-vs-controller-preprod"
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
        backend_group_id  = "a5dmfgc90mghb2pgc4f9"
        retry_policy {
          interval    = "1s"
          num_retries = 2
          retry_on = [
            "INTERNAL",
            "CANCELLED",
            "UNAVAILABLE",
            "RESOURCE_EXHAUSTED",
            "DEADLINE_EXCEEDED",
          ]
        }
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-vs-analyzer-preprod:
resource "ycp_platform_alb_virtual_host" "vh-vulnerability-scanner-analyzer-preprod" {
  authority = [
    "vs-analyzer.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = "a5d39v77dpdjqsr001m4"
  name           = "vh-vs-analyzer-preprod"
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
        backend_group_id  = "a5dh68qhi7kkl1vq3r3q"
        retry_policy {
          interval    = "1s"
          num_retries = 2
          retry_on = [
            "INTERNAL",
            "CANCELLED",
            "UNAVAILABLE",
            "RESOURCE_EXHAUSTED",
            "DEADLINE_EXCEEDED",
          ]
        }
      }
    }
  }
}

# ycp_platform_alb_virtual_host.vh-instance-group-preprod:
resource "ycp_platform_alb_virtual_host" "vh-instance-group-preprod" {
  authority = [
    "instance-group.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-instance-group-preprod"
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
        backend_group_id  = "a5dpkk5edpiadg23vjrg"
        max_timeout       = "60s"

        retry_policy {
          interval    = "1s"
          num_retries = 2
          retry_on = [
            "INTERNAL",
            "CANCELLED",
            "UNAVAILABLE",
            "DEADLINE_EXCEEDED",
          ]
        }
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-iot-devices-preprod:
resource "ycp_platform_alb_virtual_host" "vh-iot-devices-preprod" {
  authority = [
    "iot-devices.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-iot-devices-preprod"
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
        backend_group_id  = "a5dhatjrbuisomdbpslc"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-k8s-api-preprod:
resource "ycp_platform_alb_virtual_host" "vh-k8s-api-preprod" {
  authority = [
    "mk8s.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-k8s-api-preprod"
  ports = [
    443,
  ]
  route {
    name = "SwitchController"

    grpc {
      match {
        fqmn {
          exact_match = "/yandex.cloud.priv.k8s.v1.inner.ClusterService/SwitchController"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "a5dmeb8v7npdpd6phhib"
        idle_timeout      = "5m"
      }
    }
  }


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
        backend_group_id  = "a5dmeb8v7npdpd6phhib"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-k8s-api-proxy-preprod:
resource "ycp_platform_alb_virtual_host" "vh-k8s-api-proxy-preprod" {
  authority = [
    "*.mk8s-masters.private-api.ycp.cloud-preprod.yandex.net",
    "mk8s-masters.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-k8s-api-proxy-preprod"
  ports = [
    443,
  ]

  route {
    name = "main_route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "a5d0njp61i702u987fmb"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-kms-cpl-preprod:
resource "ycp_platform_alb_virtual_host" "vh-kms-cpl-preprod" {
  authority = [
    "kms-cpl.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-kms-cpl-preprod"
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
        backend_group_id  = "a5daqldtv234od2pi465"
        max_timeout       = "10s"

        retry_policy {
          interval    = "1s"
          num_retries = 2
          retry_on = [
            "UNAVAILABLE",
          ]
        }
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-mdb-api-adapter-preprod:
resource "ycp_platform_alb_virtual_host" "vh-mdb-api-adapter-preprod" {
  authority = [
    "mdb-api-adapter.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-mdb-api-adapter-preprod"
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
        backend_group_id  = "a5dk9e74np8c453mqn9g"
      }
    }
  }
}

# ycp_platform_alb_virtual_host.vh-stt-server-private-api-preprod:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-private-api-preprod" {
  authority = [
    "stt-server.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-private-api-preprod"
  ports = [
    443,
  ]

  route {
    name = "main_route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "a5dg1d1klac7aq6cqkhr"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-private-api-preprod:
resource "ycp_platform_alb_virtual_host" "vh-stt-private-api-preprod" {
  authority = [
    "stt.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-private-api-preprod"
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
        backend_group_id  = "a5dhqkavmb7ru88ssqlp"
        idle_timeout      = "60s"
        max_timeout       = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.ycf-apigateway:
resource "ycp_platform_alb_virtual_host" "ycf-apigateway" {
  authority = [
    "serverless-gateway.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "ycf-apigateway"
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
        backend_group_id  = "a5dsinvb7e1o6jhr03p7"
        max_timeout       = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.ycf-cpl-grpc:
resource "ycp_platform_alb_virtual_host" "ycf-cpl-grpc" {
  authority = [
    "serverless-functions.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "ycf-cpl-grpc"
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
        backend_group_id  = "a5d997f0027hri69gn6b"
        max_timeout       = "60s"

        retry_policy {
          interval    = "1s"
          num_retries = 2
          retry_on = [
            "INTERNAL",
            "CANCELLED",
            "UNAVAILABLE",
            "DEADLINE_EXCEEDED",
            "RESOURCE_EXHAUSTED",
          ]
        }
      }
    }
  }
}
# ycp_platform_alb_virtual_host.ycf-cpl-https:
resource "ycp_platform_alb_virtual_host" "ycf-cpl-https" {
  authority = [
    "functions-http.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "ycf-cpl-https"
  ports = [
    443,
  ]

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
        backend_group_id   = "a5dq2c6khsbd86fkrrmo"
        support_websockets = false
        timeout            = "60s"

        retry_policy {
          interval    = "1s"
          num_retries = 2
          retry_on = [
            "CONNECT_FAILURE",
            "RETRY_5XX",
          ]
        }
      }
    }
  }
}
# ycp_platform_alb_virtual_host.ycf-packer:
resource "ycp_platform_alb_virtual_host" "ycf-packer" {
  authority = [
    "serverless-packer.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "ycf-packer"
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
        backend_group_id  = "a5dhosqkj426cnpvu9h7"
        idle_timeout      = "5m"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.ycf-scheduler:
resource "ycp_platform_alb_virtual_host" "ycf-scheduler" {
  authority = [
    "serverless-scheduler.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "ycf-scheduler"
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
        backend_group_id  = "a5dht7tbhpq1mounlucg"
        max_timeout       = "60s"

        retry_policy {
          interval    = "1s"
          num_retries = 2
          retry_on = [
            "INTERNAL",
            "CANCELLED",
            "UNAVAILABLE",
            "DEADLINE_EXCEEDED",
          ]
        }
      }
    }
  }
}
# ycp_platform_alb_virtual_host.ycf-triggers:
resource "ycp_platform_alb_virtual_host" "ycf-triggers" {
  authority = [
    "serverless-triggers.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "ycf-triggers"
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
        backend_group_id  = "a5d7ikcorqh6k8kq40aq"
        idle_timeout      = "60s"

        retry_policy {
          interval    = "1s"
          num_retries = 2
          retry_on = [
            "INTERNAL",
            "CANCELLED",
            "UNAVAILABLE",
            "DEADLINE_EXCEEDED",
            "RESOURCE_EXHAUSTED",
          ]
        }
      }
    }
  }
}

# ycp_platform_alb_virtual_host.log-groups-private-api:
resource "ycp_platform_alb_virtual_host" "log-events-private-api" {
  authority = [
    "log-events.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-log-events-private-api-preprod"
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
        backend_group_id  = "a5dde7fag64tglitslbb"
      }
    }
  }
}

# ycp_platform_alb_virtual_host.log-groups-private-api:
resource "ycp_platform_alb_virtual_host" "log-groups-private-api" {
  authority = [
    "log-groups.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-log-groups-private-api-preprod"
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
        backend_group_id  = "a5d8qt95rojdgksa9u05"
      }
    }
  }
}

# ycp_platform_alb_virtual_host.vh-ui-backoffice-l7-preprod:
resource "ycp_platform_alb_virtual_host" "vh-ui-backoffice-l7-preprod" {
  name      = "vh-ui-backoffice-l7-preprod"
  authority = ["cloud-backoffice-preprod.cloud.yandex.ru", "backoffice-preprod.cloud.yandex.ru"]

  http_router_id = ycp_platform_alb_http_router.cpl-router.id

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
        backend_group_id   = "a5dnpp2bgcb60u9j728e"
        idle_timeout       = "60s"
        support_websockets = true
        timeout            = "60s"
      }
    }
  }
}

# ycp_platform_alb_virtual_host.vh-datalens-preprod-us:
resource "ycp_platform_alb_virtual_host" "vh-datalens-preprod-us" {
  authority = [
    "us-dl.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-datalens-preprod-us"
  ports = [
    443,
  ]
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
        backend_group_id   = "a5dn1ug6gjf8u6j0nk87"
        support_websockets = false
      }
    }
  }
}

# ycp_platform_alb_virtual_host.vh-trail-cpl-preprod:
resource "ycp_platform_alb_virtual_host" "vh-trail-cpl-preprod" {
  authority = [
    "trail.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-trail-cpl-preprod"
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
        backend_group_id  = "a5douj71c0uaii0um4g2"

        retry_policy {
          interval    = "1s"
          num_retries = 2
          retry_on = [
            "UNAVAILABLE",
          ]
        }
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-mdbproxy-serverless-preprod:
resource "ycp_platform_alb_virtual_host" "vh-mdbproxy-serverless-preprod" {
  authority = [
    "mdbproxy-cpl.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-mdbproxy-serverless-preprod"
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
        backend_group_id  = "a5dmjs5dl5mh35sjm7sj"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-yc-search-proxy-preprod
resource "ycp_platform_alb_virtual_host" "vh-yc-search-proxy-preprod" {
  authority = [
    "yc-search-proxy.private-api.ycp.cloud-preprod.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-yc-search-proxy-preprod"
  ports = [
    443,
  ]

  route {
    name = "main_route"
    http {
      match {
        path {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "a5d8s4rjbv4suo6f0h4i"
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "logs_cpl" {
  authority = [
    "logs-cpl.private-api.ycp.cloud-preprod.yandex.net",
    "logging-cpl.private-api.ycp.cloud-preprod.yandex.net"
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "logs-cpl-vh"
  ports = [
    443,
  ]
  route {
    name = "main"
    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "a5doatd7l0gm4hsak60e"
        retry_policy {
          num_retries = "2"
          interval    = "1s"
          retry_on = [
            "INTERNAL",
            "CANCELLED",
            "UNAVAILABLE",
            "RESOURCE_EXHAUSTED",
            "DEADLINE_EXCEEDED",
          ]
        }
      }
    }
  }
}
