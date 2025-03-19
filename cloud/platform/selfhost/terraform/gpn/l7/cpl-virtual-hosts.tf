resource "ycp_platform_alb_http_router" "cpl-router" {
  # id = fke835fqmh5jeputd8dh
  name = "cpl"
}

resource "ycp_platform_alb_virtual_host" "cpl-test" {
  authority      = ["test.private-api.ycp.gpn.yandexcloud.net"]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "cpl-test"

  route {
    name = "admin"

    http {
      match {
        path {
          exact_match = "/memory"
        }
      }

      route {
        backend_group_id = ycp_platform_alb_backend_group.admin.id
      }
    }
  }

  route {
    name = "default"

    http {
      direct_response {
        status = 200
        body {
          text = "OK"
        }
      }
    }
  }
}

resource "ycp_platform_alb_target_group" "localhost" {
  name      = "localhost"
  folder_id = var.folder_id

  target {
    ip_address = "127.0.0.1"
  }
}

resource "ycp_platform_alb_backend_group" "admin" {
  name      = "admin"
  folder_id = var.folder_id

  http {
    backend {
      name = "default"
      port = 9901
      target_group {
        target_group_id = ycp_platform_alb_target_group.localhost.id
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-api-adapter-gpn" {
  authority = [
    "api-adapter.private-api.ycp.gpn.yandexcloud.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-api-adapter-gpn"
  ports          = [443]
  route {
    name = "vpc-api"
    grpc {
      match {
        fqmn { prefix_match = "/yandex.cloud.priv.vpc.v1." }
      }
      route {
        backend_group_id = "fkemg2gl6f4fsvkpnv5r"
      }
    }
  }
  route {
    name = "loadbalancer-api"
    grpc {
      match {
        fqmn { prefix_match = "/yandex.cloud.priv.loadbalancer.v1." }
      }
      route {
        backend_group_id = "fkemg2gl6f4fsvkpnv5r"
      }
    }
  }
  route {
    name = "compute-api"
    grpc {
      match {
        fqmn { prefix_match = "/yandex.cloud.priv.compute.v1." }
      }
      route {
        backend_group_id = "fkeib4hli4vi8n9ptgnd"
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
        backend_group_id = "albe7rsnidtn9a7saa53"
        max_timeout      = "60s"
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

resource "ycp_platform_alb_virtual_host" "vh-instance-group-gpn" {
  authority = [
    "instance-group.private-api.ycp.gpn.yandexcloud.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-instance-group-gpn"
  ports          = [443]
  route {
    name = "main_route"
    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "albhena3c8e0e57jpmm5"
        max_timeout      = "60s"
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

resource "ycp_platform_alb_virtual_host" "vh-vpc-api-gpn" {
  authority = [
    "network-api-internal.private-api.ycp.gpn.yandexcloud.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-vpc-api-gpn"
  ports          = [443]
  route {
    name = "main_route"
    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "fkemg2gl6f4fsvkpnv5r"
        max_timeout      = "60s"
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

resource "ycp_platform_alb_virtual_host" "vh-hcaas-gpn" {
  authority = [
    "hcaas.private-api.ycp.gpn.yandexcloud.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-hcaas-gpn"
  ports          = [443]
  route {
    name = "main_route"
    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "fkedkenkiuajlv27tlnd"
        max_timeout      = "60s"
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

resource "ycp_platform_alb_virtual_host" "vh-access-service-gpn" {
  authority = [
    "as.private-api.ycp.gpn.yandexcloud.net"
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-access-service-gpn"
  route {
    name = "main_route"
    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "fkef8qe0jdir4t92rulm"
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-iam-control-plane-gpn" {
  authority = [
    "iam.private-api.ycp.gpn.yandexcloud.net"
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-iam-control-plane-gpn"
  route {
    name = "main_route"
    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "fkea1o27meu21e1mfk0v"
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-openid-server-gpn" {
  authority = [
    "oauth.private-api.ycp.gpn.yandexcloud.net"
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-openid-server-gpn"
  route {
    name = "main_route"
    http {
      match {
        path {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "fkenh53h1teo5t6r38lq"
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-session-service-gpn" {
  authority = [
    "ss.private-api.ycp.gpn.yandexcloud.net"
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-session-service-gpn"
  route {
    name = "main_route"
    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "fke2q87bcc11tlh38hga"
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-resource-manager-gpn" {
  authority = [
    "reaper.private-api.ycp.gpn.yandexcloud.net"
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-resource-manager-gpn"
  route {
    name = "main_route"
    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "fkea7h7sqp0rdeg4umqf"
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-rm-control-plane-gpn" {
  authority = [
    "rm.private-api.ycp.gpn.yandexcloud.net"
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-rm-control-plane-gpn"
  route {
    name = "main_route"
    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "fkenuh8qc762bjbvvbs4"
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-token-service-gpn" {
  authority = [
    "ts.private-api.ycp.gpn.yandexcloud.net"
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-token-service-gpn"
  route {
    name = "main_route"
    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "fke36s7ed9qvglmafc0b"
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-org-service-gpn" {
  authority = [
    "org.private-api.ycp.gpn.yandexcloud.net"
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-org-service-gpn"
  route {
    name = "main_route"
    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "fkelrs8cp261millt25j"
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-identity-private-gpn" {
  authority = [
    "identity.private-api.ycp.gpn.yandexcloud.net"
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-identity-private-gpn"
  route {
    name = "main_route"
    http {
      match {
        path {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "fkeadin9r1pkrul48ct7"
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-monitoring-gpn" {
  name      = "vh-monitoring-gpn"
  authority = ["monitoring.private-api.ycp.gpn.yandexcloud.net"]

  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  route {
    name = "api_v2"
    http {
      match {
        path { prefix_match = "/api/v2" }
      }
      route {
        backend_group_id = "fkes939n1kdpch3p6q66"
      }
    }
  }
  route {
    name = "monitoring_v2"
    http {
      match {
        path { prefix_match = "/monitoring/v2" }
      }
      route {
        backend_group_id = "fkes939n1kdpch3p6q66"
      }
    }
  }
  route {
    name = "monitoring_private_grpc_v2"
    grpc {
      match {
        fqmn { prefix_match = "/yandex.cloud.priv.monitoring.v2" }
      }
      route {
        backend_group_id = "fke0s5hojv0n1aiumnct"
      }
    }
  }
  route {
    name = "monitoring_gprc_v3"
    grpc {
      match {
        fqmn { prefix_match = "/yandex.monitoring.v3" }
      }
      route {
        backend_group_id = "fke0s5hojv0n1aiumnct"
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-solomon-ui-gpn" {
  name      = "vh-solomon-ui-gpn"
  authority = ["solomon.ycp.gpn.yandexcloud.net"]

  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  route {
    name = "default"
    http {
      match {
        path { prefix_match = "/" }
      }
      route {
        backend_group_id = "fkeg0fvg7agqnqf06or8"
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-compute-api" {
  name      = "vh-compute-api"
  authority = ["compute-api.private-api.ycp.gpn.yandexcloud.net"]

  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  route {
    name = "main_route"
    grpc {
      match {
        fqmn {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "fkeib4hli4vi8n9ptgnd"
        max_timeout      = "60s"
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

resource "ycp_platform_alb_virtual_host" "vh-compute-iaas" {
  name      = "vh-compute-iaas"
  authority = ["iaas.private-api.ycp.gpn.yandexcloud.net"]

  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  route {
    name = "main_route"
    http {
      match {
        path {
          prefix_match = "/"
        }
      }
      route {
        backend_group_id = "fkednpc714u29l6g8agk"
      }
    }
  }
}

resource "ycp_platform_alb_virtual_host" "vh-container-registry-gpn" {
  authority = [
    "container-registry.private-api.ycp.gpn.yandexcloud.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-container-registry-gpn"
  ports          = [443]

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
        backend_group_id  = "fke62e7le3u465b9cu83"

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

# ycp_platform_alb_virtual_host.vh-mk8s-private-api-gpn:
resource "ycp_platform_alb_virtual_host" "vh-mk8s-private-api-gpn" {
  authority = [
    "mk8s.private-api.ycp.gpn.yandexcloud.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-mk8s-private-api-gpn"
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
        backend_group_id  = "fker1e1uifl0k35pb14d"
      }
    }
  }
}
# ycp_platform_alb_virtual_host.vh-k8s-api-proxy-gpn:
resource "ycp_platform_alb_virtual_host" "vh-k8s-api-proxy-gpn" {
  authority = [
    "*.mk8s-masters.private-api.ycp.gpn.yandexcloud.net",
    "mk8s-masters.private-api.ycp.gpn.yandexcloud.net",
  ]
  http_router_id = ycp_platform_alb_http_router.cpl-router.id
  name           = "vh-k8s-api-proxy-gpn"
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
        backend_group_id  = "fke1bsmp7rkgrkmjvrgs"
      }
    }
  }
}
