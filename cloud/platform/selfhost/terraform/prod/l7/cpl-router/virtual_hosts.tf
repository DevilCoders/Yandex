locals {
  compute_grpc_api_internal_bg        = "ds7gogpe5c9gfd3dve8p" # https://st.yandex-team.ru/CLOUD-52522
  compute_grpc_operations_internal_bg = "ds7unaivf433noftqcf4" # https://st.yandex-team.ru/CLOUD-52522
  vpc_api_internal_bg                 = "ds7qnen2bl8four293el"
  hcaas_grpc_api_internal_bg          = "ds70j788b329h76apnql" # https://st.yandex-team.ru/CLOUD-62714
  api_adapter_internal_bg             = "ds72mvrnk7b55ki34dne"
  iam_cp_grpc_api_internal_bg         = "ds79kapt4s13ul7250co" # https://st.yandex-team.ru/CLOUD-57270
  rm_cp_grpc_api_internal_bg          = "ds7jipv3qrqghhlm13ea" # https://st.yandex-team.ru/CLOUD-57270
  ts_grpc_api_internal_bg             = "ds7eitk0omhsub8cgfu0" # https://st.yandex-team.ru/CLOUD-100899
}

resource "ycp_platform_alb_virtual_host" "terraform-ycp" {
  authority = [
    "terraform.ycp.cloud.yandex.net",
    "terraform.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "terraform-ycp"

  route {
    name = "well-known"

    http {
      match {
        path { exact_match = "/.well-known/terraform.json" }
      }

      direct_response {
        status = 200
        body {
          text = jsonencode({
            "providers.v1" = "https://mcdev.s3.mds.yandex.net/terraform-provider-ycp/"
          })
        }
      }
    }
  }

  modify_response_headers {
    name    = "Content-Type"
    replace = "application/json"
  }
}

# ycp_platform_alb_virtual_host.beeeye:
resource "ycp_platform_alb_virtual_host" "beeeye" {
  authority = [
    "beeeye-http.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "beeeye"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds76akdcsa2ehcgre8nu"
        idle_timeout       = "60s"
        support_websockets = false
        timeout            = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.grafana:
resource "ycp_platform_alb_virtual_host" "grafana" {
  authority = [
    "grafana.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "grafana"
  ports = [
    443,
  ]

  route {
    name = "main"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7rtl5d0hec2h8meff8"
        support_websockets = false
      }
    }
  }
}

# https://st.yandex-team.ru/CLOUD-42985
# ycp_platform_alb_virtual_host.ml-services-config:
resource "ycp_platform_alb_virtual_host" "ml-services-config" {
  authority = [
    "ml-services-config.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "ml-services-config"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds765cerjshbf67f6as1"

        retry_policy {
          interval    = "1s"
          num_retries = 3
          retry_on = [
            "INTERNAL",
          ]
        }
      }
    }
  }
}
# ycp_platform_alb_virtual_host.ml-services-config:
resource "ycp_platform_alb_virtual_host" "ml-services-config-preprod" {
  authority = [
    "ml-services-config-preprod.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "ml-services-config-preprod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7i7njl2lskocathb81"

        retry_policy {
          interval    = "1s"
          num_retries = 3
          retry_on = [
            "INTERNAL",
          ]
        }
      }
    }
  }
}
# ycp_platform_alb_virtual_host.tts-oksana:
resource "ycp_platform_alb_virtual_host" "tts-oksana" {
  authority = [
    "tts-oksana.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "tts-oksana"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7u2s5cp4fnm9gp4nv7"
        support_websockets = false
        upgrade_types      = ["protobuf"]

        retry_policy {
          interval    = "1s"
          num_retries = 3
          retry_on = [
            "RETRY_5XX",
          ]
        }
      }
    }
  }
}
# ycp_platform_alb_virtual_host.tts-oksana-rc:
resource "ycp_platform_alb_virtual_host" "tts-oksana-rc" {
  authority = [
    "tts-oksana-rc.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "tts-oksana-rc"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7bmsra6et42jrmbrim"
        support_websockets = false
        upgrade_types      = ["protobuf"]

        retry_policy {
          interval    = "1s"
          num_retries = 3
          retry_on = [
            "RETRY_5XX",
          ]
        }
      }
    }
  }
}
# ycp_platform_alb_virtual_host.tts-sergey:
resource "ycp_platform_alb_virtual_host" "tts-sergey" {
  authority = [
    "tts-sergey.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "tts-sergey"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7kfukrcmlfg4lt3d0b"
        support_websockets = false
        upgrade_types      = ["protobuf"]

        retry_policy {
          interval    = "1s"
          num_retries = 3
          retry_on = [
            "RETRY_5XX",
          ]
        }
      }
    }
  }
}
# ycp_platform_alb_virtual_host.tts-sergey-no-billing:
resource "ycp_platform_alb_virtual_host" "tts-sergey-no-billing" {
  authority = [
    "tts-sergey-no-billing.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "tts-sergey-no-billing"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds739dh298u6qm4ddq9i"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.tts-service-alb-preprod:
resource "ycp_platform_alb_virtual_host" "tts-service-alb-preprod" {
  authority = [
    "tts-service-alb-preprod.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "tts-service-alb-preprod"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds74nucpc8p2gt9nlp43"
        support_websockets = false
        upgrade_types      = ["protobuf"]

        retry_policy {
          interval    = "1s"
          num_retries = 3
          retry_on = [
            "RETRY_5XX",
          ]
        }
      }
    }
  }
}
# ycp_platform_alb_virtual_host.tts-valtz:
resource "ycp_platform_alb_virtual_host" "tts-valtz" {
  authority = [
    "tts-valtz.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "tts-valtz"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds72e39105vokltc6hum"
        support_websockets = false
        upgrade_types      = ["protobuf"]

        retry_policy {
          interval    = "1s"
          num_retries = 3
          retry_on = [
            "RETRY_5XX",
          ]
        }
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-api-adapter-prod:
resource "ycp_platform_alb_virtual_host" "vh-api-adapter-prod" {
  authority = [
    "api-adapter.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-api-adapter-prod"
  ports = [
    443,
  ]

  # https://st.yandex-team.ru/CLOUD-52522
  # Route specific gRPC requests to Compute API directly (operations).
  route {
    name = "compute-operations"

    grpc {
      match {
        fqmn {
          prefix_match = "/yandex.cloud.priv.compute.v1.OperationService/"
        }
      }

      route {
        backend_group_id = local.compute_grpc_operations_internal_bg
      }
    }
  }

  # https://st.yandex-team.ru/CLOUD-52522
  # Route specific gRPC requests to Compute API directly (rest of methods)
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
        backend_group_id = local.vpc_api_internal_bg
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
        backend_group_id = local.vpc_api_internal_bg
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

  # y.c.p.resourcemanager.v1.* -> ResourceManager Control Plane gRPC API backend
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
# virtual host 1 to test new compute backend group
resource "ycp_platform_alb_virtual_host" "vh-compute-api-prod" {
  authority = [
    "compute-api.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-compute-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = local.compute_grpc_api_internal_bg
      }
    }
  }
}
# virtual host 2 to test new compute backend group
resource "ycp_platform_alb_virtual_host" "vh-compute-operations-prod" {
  authority = [
    "compute-operations.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-compute-operations-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = local.compute_grpc_operations_internal_bg
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-container-registry-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-container-registry-private-api-prod" {
  authority = [
    "container-registry.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-container-registry-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds703jbrs65kh0502nue"

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
# ycp_platform_alb_virtual_host.vh-vs-controller-prod:
resource "ycp_platform_alb_virtual_host" "vh-vulnerability-scanner-controller-prod" {
  authority = [
    "vs-controller.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-vs-controller-prod"
  ports = [
    443,
  ]
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
        backend_group_id  = "ds7lhk115v0oprm3dajn"
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
# ycp_platform_alb_virtual_host.vh-vs-analyzer-prod:
resource "ycp_platform_alb_virtual_host" "vh-vulnerability-scanner-analyzer-prod" {
  authority = [
    "vs-analyzer.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-vs-analyzer-prod"
  ports = [
    443,
  ]
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
        backend_group_id  = "ds7q6276kb4vucbao6be"
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
# ycp_platform_alb_virtual_host.vh-datalens-staging-dataset-api:
resource "ycp_platform_alb_virtual_host" "vh-datalens-staging-dataset-api" {
  authority = [
    "datalens-staging-dataset.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-datalens-staging-dataset-api"
  ports = [
    443,
  ]

  route {
    name = "materializer-ping"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/materializer/ping"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7npbkhfk83o2eoi1q9"
        prefix_rewrite     = "/ping"
        support_websockets = false
      }
    }
  }
  route {
    name = "materializer"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/materializer"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7npbkhfk83o2eoi1q9"
        support_websockets = false
      }
    }
  }
  route {
    name = "dataset-data-api"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/api/data/v1"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7h9nb49gmjh1o5ghn7"
        prefix_rewrite     = "/api/v1"
        support_websockets = false
      }
    }
  }
  route {
    name = "public-api-v1"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/public/api/v1"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7315d8dtevri2paul0"
        prefix_rewrite     = "/api/v1"
        support_websockets = false
      }
    }
  }
  route {
    name = "public-api-data-v1"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/public/api/data/v1"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7315d8dtevri2paul0"
        prefix_rewrite     = "/api/v1"
        support_websockets = false
      }
    }
  }
  route {
    name = "main-route"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds70ctrdtegv2bq3m18p"
        support_websockets = false
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-instance-group-prod:
resource "ycp_platform_alb_virtual_host" "vh-instance-group-prod" {
  authority = [
    "instance-group.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-instance-group-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7ofv5q6vncmc75m64o"
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
# ycp_platform_alb_virtual_host.vh-iot-devices-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-iot-devices-private-api-prod" {
  authority = [
    "iot-devices.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-iot-devices-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds77k0vvaq4lsi3bdhjh"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-kms-cpl-prod:
resource "ycp_platform_alb_virtual_host" "vh-kms-cpl-prod" {
  authority = [
    "kms-cpl.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-kms-cpl-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7u6j396hc7s4g4jbhv"
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
# ycp_platform_alb_virtual_host.vh-mdb-api-adapter-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-mdb-api-adapter-private-api-prod" {
  authority = [
    "mdb-api-adapter.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-mdb-api-adapter-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7stike52r9iv7dto6p"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-mk8s-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-mk8s-private-api-prod" {
  authority = [
    "mk8s.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-mk8s-private-api-prod"
  ports = [
    443,
  ]

  route {
    name = "inner-api"

    grpc {
      match {
        fqmn {
          prefix_match = "/yandex.cloud.priv.k8s.v1.inner."
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds79dkmvgosrll1mahhh"
        idle_timeout      = "5m"
      }
    }
  }

  route {
    name = "inner-cluster-service-list"

    grpc {
      match {
        fqmn {
          exact_match = "/yandex.cloud.priv.k8s.v1.inner.ClusterService/List"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds79dkmvgosrll1mahhh"
        idle_timeout      = "5m"
      }
    }
  }

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
        backend_group_id  = "ds79dkmvgosrll1mahhh"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-k8s-api-proxy-prod:
resource "ycp_platform_alb_virtual_host" "vh-k8s-api-proxy-prod" {
  authority = [
    "*.mk8s-masters.private-api.ycp.cloud.yandex.net",
    "mk8s-masters.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-k8s-api-proxy-prod"
  ports = [
    443,
  ]
  route {
    name = "main-route"
    http {
      match {
        path {
          prefix_match = "/"
        }
      }
      route {
        auto_host_rewrite = false
        backend_group_id  = "ds7arl5bdq5p7hrrk498"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-services-proxy-stt-preprod-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-services-proxy-stt-preprod-private-api-prod" {
  authority = [
    "services-proxy-stt-preprod.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-services-proxy-stt-preprod-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7kf85ufpb7lnnfdra6"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-services-proxy-tts-preprod-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-services-proxy-tts-preprod-prod" {
  authority = [
    "services-proxy-tts-preprod.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-services-proxy-tts-preprod-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7trbg4djq1smh3cn17"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-services-proxy-vision-preprod-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-services-proxy-vision-preprod-private-api-prod" {
  authority = [
    "services-proxy-vision-preprod.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-services-proxy-vision-preprod-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds74oe8fl38063s2rhge"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-services-proxy-locator-preprod-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-services-proxy-locator-preprod-private-api-prod" {
  authority = [
    "services-proxy-locator-preprod.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-services-proxy-locator-preprod-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7hl661lkgbjimu9t0k"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-services-proxy-translate-preprod-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-services-proxy-translate-preprod-private-api-prod" {
  authority = [
    "services-proxy-translate-preprod.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-services-proxy-translate-preprod-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7mblmmtdsvq2g6a0qe"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-services-proxy-stt-rest-preprod-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-services-proxy-stt-rest-preprod-private-api-prod" {
  authority = [
    "services-proxy-stt-rest-preprod.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-services-proxy-stt-rest-preprod-private-api-prod"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds7urhbubaa7gl5tbg3n"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-services-proxy-tts-rest-preprod-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-services-proxy-tts-rest-preprod-private-api-prod" {
  authority = [
    "services-proxy-tts-rest-preprod.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-services-proxy-tts-rest-preprod-private-api-prod"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds7rpg2dg3qbo5uav7hh"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-services-proxy-translate-rest-preprod-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-services-proxy-translate-rest-preprod-private-api-prod" {
  authority = [
    "services-proxy-translate-rest-preprod.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-services-proxy-translate-rest-preprod-private-api-prod"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds7gq8hb2fvoevsvl8br"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-services-proxy-stt-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-services-proxy-stt-private-api-prod" {
  authority = [
    "services-proxy-stt.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-services-proxy-stt-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7mss1bj25ncbcmrpgq"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-services-proxy-translate-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-services-proxy-translate-private-api-prod" {
  authority = [
    "services-proxy-translate.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-services-proxy-translate-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7nv2la36c8oqsvvu9e"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-services-proxy-locator-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-services-proxy-locator-private-api-prod" {
  authority = [
    "services-proxy-locator.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-services-proxy-locator-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7qi657t1pnki8vm1lk"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-services-proxy-vision-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-services-proxy-vision-private-api-prod" {
  authority = [
    "services-proxy-vision.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-services-proxy-vision-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7rgib7lprbtvo1c3ha"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-services-proxy-stt-rest-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-services-proxy-stt-rest-private-api-prod" {
  authority = [
    "services-proxy-stt-rest.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-services-proxy-stt-rest-private-api-prod"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds7sgm5eh1bksrq0olv0"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-services-proxy-tts-rest-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-services-proxy-tts-rest-private-api-prod" {
  authority = [
    "services-proxy-tts-rest.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-services-proxy-tts-rest-private-api-prod"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds7pghde8g2nqfc25k86"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-services-proxy-translate-rest-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-services-proxy-translate-rest-private-api-prod" {
  authority = [
    "services-proxy-translate-rest.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-services-proxy-translate-rest-private-api-prod"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds7nascuvlvincrqrkji"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-server-private-api-staging:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-private-api-staging" {
  authority = [
    "stt-server-preprod.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-private-api-staging"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds7lmbj53j54mlgv7f5u"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-server-cpu-private-api-staging:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-cpu-private-api-staging" {
  authority = [
    "stt-server-cpu-preprod.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-cpu-private-api-staging"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds7t5haerbc5q32qd1mj"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-server-grpc-private-api-staging:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-grpc-private-api-staging" {
  authority = [
    "stt-server-grpc-preprod.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-grpc-private-api-staging"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7hogb4ason4dlqtoir"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-server-aurelius-hotfix1-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-aurelius-hotfix1-private-api-prod" {
  authority = [
    "stt-server-aurelius-hotfix1.private-api.ycp.cloud.yandex.net",
    "stt-server-general-deprecated.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-aurelius-hotfix1-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7kauoh316mdj1gbbah"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-server-no-lm-general-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-no-lm-general-private-api-prod" {
  authority = [
    "stt-server-no-lm-general.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-no-lm-general-private-api-prod"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds7aoggh6gbnnbf84kgv"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-server-no-lm-general-rc-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-no-lm-general-rc-private-api-prod" {
  authority = [
    "stt-server-no-lm-general-rc.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-no-lm-general-rc-private-api-prod"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds7tvc05tqija1ev2i4g"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-server-biovitrum-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-biovitrum-private-api-prod" {
  authority = [
    "stt-server-biovitrum.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-biovitrum-private-api-prod"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds7mnkmq6g4jpimg2n2m"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-server-kaspi-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-kaspi-private-api-prod" {
  authority = [
    "stt-server-kaspi.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-kaspi-private-api-prod"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds7pj5lraj7j6il26qa7"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-server-names-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-names-private-api-prod" {
  authority = [
    "stt-server-names.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-names-private-api-prod"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds76voqbn8gkqohfintk"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-server-rupor-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-rupor-private-api-prod" {
  authority = [
    "stt-server-rupor.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-rupor-private-api-prod"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds7lj0pni3run8s77o0h"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-server-anaximander-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-anaximander-private-api-prod" {
  authority = [
    "stt-server-anaximander.private-api.ycp.cloud.yandex.net",
    "stt-server-general.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-anaximander-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7c3cc028dneckk9ra1"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-server-general-rc-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-general-rc-api-prod" {
  authority = [
    "stt-server-general-rc.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-general-rc-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds755vg9tatpipjp7hp1"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-server-grpc-general-rc-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-grpc-general-rc-api-prod" {
  authority = [
    "stt-server-grpc-general-rc.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-grpc-general-rc-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7davt4rn9255s6enip"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-server-grpc-general-v3-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-grpc-general-v3-api-prod" {
  authority = [
    "stt-server-grpc-general-v3.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-grpc-general-v3-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7c3cc028dneckk9ra1"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-server-hqa-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-hqa-private-api-prod" {
  authority = [
    "stt-server-hqa.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-hqa-private-api-prod"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds7omvsb9p7ol05gnosk"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-server-kazah-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-stt-server-kazah-private-api-prod" {
  authority = [
    "stt-server-kazah.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-server-kazah-private-api-prod"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {
      match {
        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds7fd8tlcjin507grs6j"
        idle_timeout      = "60s"
        upgrade_types     = ["protobuf"]
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-translation-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-translation-private-api-prod" {
  authority = [
    "translation.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-translation-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7fb0dla7oa9t381av9"
        idle_timeout      = "60s"
        max_timeout       = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-feedback-private-api-staging:
resource "ycp_platform_alb_virtual_host" "vh-stt-feedback-private-api-staging" {
  authority = [
    "stt-feedback-preprod.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-feedback-private-api-staging"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds72fj1lfk3o1qvci4tp"
        idle_timeout      = "600s"
        max_timeout       = "600s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-stt-feedback-private-api-prod:
resource "ycp_platform_alb_virtual_host" "vh-stt-feedback-private-api-prod" {
  authority = [
    "stt-feedback.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-stt-feedback-private-api-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7cdj3q43gou3qqm5sq"
        idle_timeout      = "600s"
        max_timeout       = "600s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-node-deployer-preprod:
resource "ycp_platform_alb_virtual_host" "vh-node-deployer-preprod" {
  authority = [
    "node-deployer-preprod.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-node-deployer-preprod-private-api"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7ojuhrrtt3tpar735l"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-node-deployer-prod:
resource "ycp_platform_alb_virtual_host" "vh-node-deployer-prod" {
  authority = [
    "node-deployer.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-node-deployer-prod-private-api"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7mfl1v3ave9iqvtcp0"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-node-service-preprod:
resource "ycp_platform_alb_virtual_host" "vh-node-service-preprod" {
  authority = [
    "node-service-preprod.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-node-service-preprod-private-api"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7074titfu1sd4hiddo"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-node-service-prod:
resource "ycp_platform_alb_virtual_host" "vh-node-service-prod" {
  authority = [
    "node-service.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-node-service-prod-private-api"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7imgbbqle1mr4ukaoe"
        idle_timeout      = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-ui-l7-prod:
resource "ycp_platform_alb_virtual_host" "vh-ui-l7-prod" {
  authority = [
    "*.front-intprod.cloud.yandex.ru",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-ui-l7-prod"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds78b34e8bknecgd15r6"
        idle_timeout       = "60s"
        support_websockets = false
        timeout            = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.ycf-apigateway:
resource "ycp_platform_alb_virtual_host" "ycf-apigateway" {
  authority = [
    "serverless-gateway.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "ycf-apigateway"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7pa3ulne68tq6dp9ah"
        max_timeout       = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.ycf-cpl-http:
resource "ycp_platform_alb_virtual_host" "ycf-cpl-http" {
  authority = [
    "functions-http.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "ycf-cpl-http"
  ports = [
    443,
  ]

  route {
    name = "main-route"

    http {

      match {
        http_method = []

        path {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds713ictlhvplmu0comi"
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
# ycp_platform_alb_virtual_host.ycf-cpl:
resource "ycp_platform_alb_virtual_host" "ycf-cpl" {
  authority = [
    "serverless-functions.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "ycf-cpl"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds78u3tfan6alihqg0cv"
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
# ycp_platform_alb_virtual_host.ycf-packer:
resource "ycp_platform_alb_virtual_host" "ycf-packer" {
  authority = [
    "serverless-packer.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "ycf-packer"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7ld556l4mci61mfiru"
        idle_timeout      = "600s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.ycf-scheduler:
resource "ycp_platform_alb_virtual_host" "ycf-scheduler" {
  authority = [
    "serverless-scheduler.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "ycf-scheduler"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds766cn9u17e602ea404"
        max_timeout       = "600s"

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
    "serverless-triggers.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "ycf-triggers"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds725v7hv5oitj4mh0lt"
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
# ycp_platform_alb_virtual_host.vh-dns-prod:
resource "ycp_platform_alb_virtual_host" "vh-dns-prod" {
  authority = [
    "dns.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-dns-prod"
  ports = [
    443,
  ]

  route {
    name = "proxy-route"

    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds77n3kkt00ja17bej9m"
        max_timeout       = "180s"
        idle_timeout      = "180s"

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

# ycp_platform_alb_virtual_host.vh-dns-lab-prod-hwlab:
resource "ycp_platform_alb_virtual_host" "vh-dns-lab-prod" {
  authority = [
    "dns-lab.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-dns-lab-prod"
  ports = [
    443,
  ]

  route {
    name = "proxy-route"

    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = "ds77iknetrc8o2q2vgjj"
        max_timeout       = "180s"
        idle_timeout      = "180s"

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

# ycp_platform_alb_virtual_host.log-groups-private-api:
resource "ycp_platform_alb_virtual_host" "log-events-private-api" {
  authority = [
    "log-events.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-log-events-private-api"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7jdgtb8st02aqjcj1q"
      }
    }
  }
}

# ycp_platform_alb_virtual_host.log-groups-private-api:
resource "ycp_platform_alb_virtual_host" "log-groups-private-api" {
  authority = [
    "log-groups.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-log-groups-private-api"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7l2grnf87l8i62od93"
      }
    }
  }
}


# ycp_platform_alb_virtual_host.data-transfer-grpc-private-api-preprod:
resource "ycp_platform_alb_virtual_host" "data-transfer-grpc-private-api-preprod" {
  authority = [
    "data-transfer.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "data-transfer-grpc-private-api-preprod"
  ports = [
    443,
  ]
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
        backend_group_id  = "ds7vfjq3cacgqm9r0cpj"
        idle_timeout      = "60s"
        max_timeout       = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.data-transfer-rest-private-api-preprod:
resource "ycp_platform_alb_virtual_host" "data-transfer-rest-private-api-preprod" {
  authority = [
    "data-transfer-rest.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "data-transfer-rest-private-api-preprod"
  ports = [
    443,
  ]
  route {
    name = "main-route"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/"
        }
      }
      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7dgce0hbkflu63c76i"
        idle_timeout       = "60s"
        support_websockets = false
        timeout            = "60s"
      }
    }
  }
}

# ycp_platform_alb_virtual_host.vh-ui-backoffice-l7-prod:
resource "ycp_platform_alb_virtual_host" "vh-ui-backoffice-l7-prod" {
  name      = "vh-ui-backoffice-l7-prod"
  authority = ["cloud-backoffice.cloud.yandex.ru", "backoffice.cloud.yandex.ru"]

  http_router_id = ycp_platform_alb_http_router.cpl-router.id

  route {
    name = "main-route"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/"
        }
      }
      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds77bm4pj9ihno63tkih"
        idle_timeout       = "60s"
        support_websockets = true
        timeout            = "60s"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-trail-cpl-prod:
resource "ycp_platform_alb_virtual_host" "vh-trail-cpl-prod" {
  authority = [
    "trail.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-trail-cpl-prod"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds75fp2ct4qqc3rm0i40"
        max_timeout       = "10s"

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

# ycp_platform_alb_virtual_host.vh-datalens-prod-us:
resource "ycp_platform_alb_virtual_host" "vh-datalens-prod-us" {
  authority = [
    "us-dl.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-datalens-prod-us"
  ports = [
    443,
  ]
  route {
    name = "main-route"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/"
        }
      }
      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7g9g7nqgdaj88jet8r"
        support_websockets = false
      }
    }
  }
}

# ycp_platform_alb_virtual_host.vh-datalens-prod-back-api:
resource "ycp_platform_alb_virtual_host" "vh-datalens-prod-back-api" {
  authority = [
    "datalens-back.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-datalens-prod-back-api"
  ports = [
    443,
  ]
  route {
    name = "materializer-ping"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/materializer/ping"
        }
      }
      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7n6ha6d5vmt97fmm73"
        prefix_rewrite     = "/ping"
        support_websockets = false
        timeout            = "5s"
      }
    }
  }
  route {
    name = "materializer"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/materializer"
        }
      }
      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7n6ha6d5vmt97fmm73"
        support_websockets = false
        timeout            = "10s"
      }
    }
  }
  route {
    name = "dataset-data-api-v1"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/api/data/v1"
        }
      }
      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7ul1v9iqptqkn83975"
        support_websockets = false
        timeout            = "95s"
      }
    }
  }
  route {
    name = "dataset-data-api-v1-5"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/api/data/v1.5"
        }
      }
      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7ul1v9iqptqkn83975"
        support_websockets = false
        timeout            = "95s"
      }
    }
  }
  route {
    name = "dataset-data-api-v2"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/api/data/v2"
        }
      }
      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7ul1v9iqptqkn83975"
        support_websockets = false
        timeout            = "95s"
      }
    }
  }
  route {
    name = "public-api-data-v1"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/public/api/data/v1"
        }
      }
      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7nvlmjohut67t62uou"
        prefix_rewrite     = "/api/data/v1"
        support_websockets = false
        timeout            = "95s"
      }
    }
  }
  route {
    name = "public-api-data-v1-5"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/public/api/data/v1.5"
        }
      }
      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7nvlmjohut67t62uou"
        prefix_rewrite     = "/api/data/v1.5"
        support_websockets = false
        timeout            = "95s"
      }
    }
  }
  route {
    name = "public-api-data-v2"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/public/api/data/v2"
        }
      }
      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7nvlmjohut67t62uou"
        prefix_rewrite     = "/api/data/v2"
        support_websockets = false
        timeout            = "95s"
      }
    }
  }
  route {
    name = "billing-api"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/billing"
        }
      }
      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds73pf713rv5i8iofrt7"
        prefix_rewrite     = "/api"
        support_websockets = false
        timeout            = "5s"
      }
    }
  }
  route {
    name = "setup-folder-api"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/folder/setup"
        }
      }
      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds73pf713rv5i8iofrt7"
        prefix_rewrite     = "/api/v1/dl_unit/setup-folder"
        support_websockets = false
        timeout            = "10s"
      }
    }
  }
  route {
    name = "file-uploader-api"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/file-uploader/"
        }
      }
      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7t63l73g95m10s0s7h"
        prefix_rewrite     = "/"
        support_websockets = false
        timeout            = "30s"
      }
    }
  }
  route {
    name = "main-route"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/"
        }
      }
      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7tc14f1oijbf1q34kq"
        support_websockets = false
        timeout            = "60s"
      }
    }
  }
}

# ycp_platform_alb_virtual_host.vh-datalens-prod-dls-api:
resource "ycp_platform_alb_virtual_host" "vh-datalens-prod-dls-api" {
  authority = [
    "datalens-dls.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-datalens-prod-dls-api"
  ports = [
    443,
  ]
  route {
    name = "main-route"
    http {
      match {
        http_method = []
        path {
          prefix_match = "/"
        }
      }
      route {
        auto_host_rewrite  = false
        backend_group_id   = "ds7k7i6s768rblr5c9en"
        support_websockets = false
        timeout            = "5s"
      }
    }
  }
}

# https://st.yandex-team.ru/CLOUD-59502
# ycp_platform_alb_virtual_host.ycp-hopper:
resource "ycp_platform_alb_virtual_host" "ycp-hopper" {
  authority = [
    "hopper.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "ycp-hopper"
  ports = [
    443,
  ]

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
        backend_group_id  = "ds7gisv7ho6ocfrjkct7"
        max_timeout       = "60s"
      }
    }
  }
}

# ycp_platform_alb_virtual_host.vh-yc-search-proxy-prod
resource "ycp_platform_alb_virtual_host" "vh-yc-search-proxy-prod" {
  authority = [
    "yc-search-proxy.private-api.ycp.cloud.yandex.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-yc-search-proxy-prod"
  ports = [
    443,
  ]

  route {
    name = "main-route"
    http {
      match {
        path {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "ds7mnp340qaadd3acsvj"
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "logging_cpl" {
  authority = [
    "logging-cpl.private-api.ycp.cloud.yandex.net"
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "logging-cpl-vh"
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
        backend_group_id = "ds7erm03lqqjst3q0f2k"
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
