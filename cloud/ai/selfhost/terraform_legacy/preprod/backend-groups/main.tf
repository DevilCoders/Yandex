data "external" "token" {
  program = ["/bin/bash", "-c", "yc iam create-token | jq -Rn '{token: input}'"]
}

provider "ycp" {
  prod  = true
  token = data.external.token.result.token
}

# ycp_platform_alb_backend_group.bg_stt_server_preprod:
resource "ycp_platform_alb_backend_group" "bg_stt_server_preprod" {
  description = "STT service ALB, staging"
  folder_id   = "b1gndji7iaubpghf15b0"
  # id        = "ds7lmbj53j54mlgv7f5u"
  name        = "stt-server-alb-staging"

  http {
    backend {
      name          = "stt-server-backend-staging-http"
      port          = 17002
      weight        = 99

      healthchecks {
        healthy_threshold   = 2
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 3

        http {
          path      = "/ping"
        }
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      target_group {
        target_group_id = "ds7nsbbhqv4t8cj87diq"
      }
    }
    connection {}
  }
}

# ycp_platform_alb_backend_group.bg_stt_server_cpu_preprod:
resource "ycp_platform_alb_backend_group" "bg_stt_server_cpu_preprod" {
  description = "STT service ALB, staging"
  folder_id   = "b1gndji7iaubpghf15b0"
  # id        = "ds7t5haerbc5q32qd1mj"
  name        = "stt-server-cpu-alb-staging"

  http {
    backend {
      name          = "stt-server-cpu-backend-staging-http"
      port          = 17002
      weight        = 99

      healthchecks {
        healthy_threshold   = 2
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 3

        http {
          path      = "/ping"
        }
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      target_group {
        target_group_id = "ds7breob5rgvqufh2lt5"
      }
    }
    connection {}
  }
}

# ycp_platform_alb_backend_group.bg_stt_server_grpc_preprod:
resource "ycp_platform_alb_backend_group" "bg_stt_server_grpc_preprod" {
  description = "STT service ALB, staging"
  folder_id   = "b1gndji7iaubpghf15b0"
  # id        = "ds7hogb4ason4dlqtoir"
  name        = "ai-stt-server-grpc-backend-group-preprod"

  grpc {
    backend {
      name   = "ai-stt-server-grpc-backend-group-preprod"
      port   = 17005
      weight = 100

      healthchecks {
        healthcheck_port    = 17005
        healthy_threshold   = 2
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 3

        grpc {}
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      tcp_options {
        connection_buffer_limit_bytes = "1048576"
      }

      target_group {
        target_group_id = "ds7nsbbhqv4t8cj87diq"
      }
    }
  }
}

# ycp_platform_alb_backend_group.bg_stt_feedback_preprod:
resource "ycp_platform_alb_backend_group" "bg_stt_feedback_preprod" {
  description = "Backend for ai-stt-feedback service (preprod)"
  folder_id   = "b1gooapscoase9vh1jip"
  # id        = "ds72fj1lfk3o1qvci4tp"
  name        = "ai-stt-feedback-backend-group-preprod"

  grpc {
    backend {
      name   = "ai-stt-feedback-preprod"
      port   = 443
      weight = 100

      healthchecks {
        healthcheck_port    = 443
        healthy_threshold   = 2
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 3

        grpc {
          service_name = "services-proxy"
        }
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      target_group {
        target_group_id = "ds711gspe1obbn77r3ea"
      }
    }
  }
}

# ycp_platform_alb_backend_group.bg_services_proxy_vision_preprod:
resource "ycp_platform_alb_backend_group" "bg_services_proxy_vision_preprod" {
  description = "Backend for ai-services-proxy-vision service (preprod)"
  folder_id   = "b1gd3ibutes0q72uq8uf"
  # id        = "ds74oe8fl38063s2rhge"
  name        = "ai-services-proxy-vision-backend-group-preprod"

  grpc {
    backend {
      name   = "ai-services-proxy-vision-preprod"
      port   = 443
      weight = 100

      healthchecks {
        healthcheck_port    = 443
        healthy_threshold   = 2
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 3

        grpc {
          service_name = "services-proxy"
        }
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      target_group {
        target_group_id = "ds7he9oc1db32esiqv5q"
      }
    }
  }
}

# ycp_platform_alb_backend_group.bg_services_proxy_translate_preprod:
resource "ycp_platform_alb_backend_group" "bg_services_proxy_translate_preprod" {
  description = "Backend for ai-services-proxy-translate service (preprod)"
  folder_id   = "b1gd3ibutes0q72uq8uf"
  # id        = "ds7mblmmtdsvq2g6a0qe"
  name        = "ai-services-proxy-translate-backend-group-preprod"

  grpc {
    backend {
      name   = "ai-services-proxy-translate-preprod"
      port   = 443
      weight = 100

      healthchecks {
        healthcheck_port    = 443
        healthy_threshold   = 2
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 3

        grpc {
          service_name = "services-proxy"
        }
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      target_group {
        target_group_id = "ds73jn8s1dcbffr0d69b"
      }
    }
  }
}

# ycp_platform_alb_backend_group.bg_services_proxy_stt_preprod:
resource "ycp_platform_alb_backend_group" "bg_services_proxy_stt_preprod" {
  description = "Backend for ai-services-proxy-stt service (preprod)"
  folder_id   = "b1gd3ibutes0q72uq8uf"
  # id        = "ds7kf85ufpb7lnnfdra6"
  name        = "ai-services-proxy-stt-backend-group-preprod"

  grpc {
    backend {
      name   = "ai-services-proxy-stt-preprod"
      port   = 443
      weight = 100

      healthchecks {
        healthcheck_port    = 443
        healthy_threshold   = 2
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 3

        grpc {
          service_name = "services-proxy"
        }
      }

      http2_options {
        max_concurrent_streams         = 100
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      target_group {
        target_group_id = "ds7fm9l9tepm6lnu0j9r"
      }

      tcp_options {
        connection_buffer_limit_bytes = 32768
      }
    }
  }
}

# ycp_platform_alb_backend_group.bg_services_proxy_tts_preprod:
resource "ycp_platform_alb_backend_group" "bg_services_proxy_tts_preprod" {
  description = "Backend for ai-services-proxy-tts service (preprod)"
  folder_id   = "b1gd3ibutes0q72uq8uf"
  # id        = "ds7trbg4djq1smh3cn17"
  name        = "ai-services-proxy-tts-backend-group-preprod"

  grpc {
    backend {
      name   = "ai-services-proxy-tts-preprod"
      port   = 443
      weight = 100

      healthchecks {
        healthcheck_port    = 443
        healthy_threshold   = 2
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 3

        grpc {
          service_name = "services-proxy"
        }
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      target_group {
        target_group_id = "ds7lniomsnmp60kcvmem"
      }
    }
  }
}

# ycp_platform_alb_backend_group.bg_services_proxy_locator_preprod:
resource "ycp_platform_alb_backend_group" "bg_services_proxy_tts_rest_preprod" {
  description = "Backend for ai-services-proxy-tts-rest service (preprod)"
  folder_id   = "b1gd3ibutes0q72uq8uf"
  # id        = "ds7rpg2dg3qbo5uav7hh"
  labels      = {}
  name        = "ai-services-proxy-tts-rest-backend-group-preprod"

  http {
    backend {
      allow_connect = false
      name          = "ai-services-proxy-tts-rest-preprod"
      port          = 80
      use_http2     = false
      weight        = 100

      healthchecks {
        healthcheck_port    = 80
        healthy_threshold   = 2
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 3

        http {
          path      = "/ping"
        }
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      target_group {
        target_group_id = "ds77efskpr794vlqptdq"
      }
    }
    connection {}
  }
}

# ycp_platform_alb_backend_group.bg_services_proxy_locator_preprod:
resource "ycp_platform_alb_backend_group" "bg_services_proxy_locator_preprod" {
  description = "Backend for ai-services-proxy-locator service (preprod)"
  folder_id   = "b1gd3ibutes0q72uq8uf"
  # id        = "ds7hl661lkgbjimu9t0k"
  name        = "ai-services-proxy-locator-backend-group-preprod"

  grpc {
    backend {
      name   = "ai-services-proxy-locator-preprod"
      port   = 443
      weight = 100

      healthchecks {
        healthcheck_port    = 443
        healthy_threshold   = 2
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 3

        grpc {
          service_name = "speechkit.stt"
        }
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      target_group {
        target_group_id = "ds7uu2o4mv4b9ov47td3"
      }
    }
  }
}

# ycp_platform_alb_backend_group.bg_services_proxy_translate_rest_preprod:
resource "ycp_platform_alb_backend_group" "bg_services_proxy_translate_rest_preprod" {
  description = "Backend for ai-services-proxy-translate-rest service (preprod)"
  folder_id   = "b1gd3ibutes0q72uq8uf"
  # id        = "ds7gq8hb2fvoevsvl8br"
  name        = "ai-services-proxy-translate-rest-backend-group-preprod"

  http {
    backend {
      name          = "ai-services-proxy-translate-rest-preprod"
      port          = 80
      weight        = 100

      healthchecks {
        healthcheck_port    = 80
        healthy_threshold   = 2
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 3

        http {
          path      = "/ping"
        }
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      target_group {
        target_group_id = "ds7c0sp3213ffnvpe9d5"
      }
    }
    connection {}
  }
}

# ycp_platform_alb_backend_group.bg_services_proxy_stt_rest_preprod:
resource "ycp_platform_alb_backend_group" "bg_services_proxy_stt_rest_preprod" {
  description = "Backend for ai-services-proxy-stt-rest service (preprod)"
  folder_id   = "b1gd3ibutes0q72uq8uf"
  # id        = "ds7urhbubaa7gl5tbg3n"
  name        = "ai-services-proxy-stt-rest-backend-group-preprod"

  http {
    backend {
      allow_connect = false
      name          = "ai-services-proxy-stt-rest-preprod"
      port          = 80
      use_http2     = false
      weight        = 100

      healthchecks {
        healthcheck_port    = 80
        healthy_threshold   = 2
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 3

        http {
          path      = "/ping"
        }
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      target_group {
        target_group_id = "ds741ckoclhdu7r76lt4"
      }
    }
    connection {}
  }
}

# ycp_platform_alb_backend_group.bg_node_deployer_service_preprod will be destroyed
resource "ycp_platform_alb_backend_group" "bg_node_deployer_service_preprod" {
  description = "Backend for ai-node-deployer service (preprod)"
  folder_id   = "b1g19hobememv3hj6qsc"
  # id          = "ds7ojuhrrtt3tpar735l"
  labels      = {}
  name        = "ai-node-deployer-service-backend-group-preprod"

  grpc {
    backend {
      name           = "ai-node-deployer-preprod"
      port           = 443
      weight         = 100

      healthchecks {
        healthcheck_port    = 443
        healthy_threshold   = 2
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 3

        grpc {
          service_name = "cloud-ai-node-deployer"
        }
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      target_group {
        target_group_id = "ds7k6ohb4d0ut03ffusf"
      }
    }
  }
}

resource "ycp_platform_alb_backend_group" "bg_node_service_preprod" {
  description = "Backend for ai-node-service (preprod)"
  folder_id   = "b1g19hobememv3hj6qsc"
  labels      = {}
  name        = "ai-node-service-backend-group-preprod"

  grpc {
    backend {
      name           = "ai-node-service-preprod"
      port           = 443
      weight         = 100

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      target_group {
        target_group_id = "ds7o16gdrr51358fd4tu"
      }
    }
  }
}


resource "ycp_platform_alb_backend_group" "bg_node_proxy_xds_preprod" {
    # id="ds7joepan43kovsai1l4"
    description = "Backend for ai-node-proxy-xds (preprod)"
    folder_id   = "b1g19hobememv3hj6qsc"
    labels      = {}
    name        = "ai-node-proxy-xds-backend-group-preprod"

    grpc {
        backend {
            name           = "ai-node-proxy-xds-preprod"
            port           = 443
            weight         = 100

            load_balancing_config {
                strict_locality                = false
                locality_aware_routing_percent = "100"
                mode                           = "ROUND_ROBIN"
                panic_threshold                = "40"
            }

            target_group {
                target_group_id = "ds7d8ifpajvg9l0pj7s9"
            }
        }
    }
}

# ycp_platform_alb_backend_group.bg_tts_server_preprod:
# terraform import ycp_platform_alb_backend_group.bg_tts_server_preprod ds74nucpc8p2gt9nlp43
# terraform state rm ycp_platform_alb_backend_group.bg_tts_server_preprod
resource "ycp_platform_alb_backend_group" "bg_tts_server_preprod" {
    description = "TTS service ALB, preprod"
    folder_id   = "b1gt5rndig3dasvkpid4"
    #id          = "ds74nucpc8p2gt9nlp43"
    labels      = {}
    name        = "tts-service-alb-preprod"

    http {
        backend {
            name           = "tts-backend-preprod"
            port           = 17004
            weight         = 100
            use_http2      = true

            healthchecks {
                healthcheck_port        = 17004
                healthy_threshold       = 2
                interval                = "2s"
                timeout                 = "1s"
                unhealthy_threshold     = 3

                grpc {}
            }

            load_balancing_config {
                locality_aware_routing_percent = 100
                mode                           = "ROUND_ROBIN"
                panic_threshold                = 40
                strict_locality                = false
            }

            target_group {
                target_group_id = "ds732t1po5g2rjmt6bae"
            }
        }
    }
}

/*
resource "ycp_platform_alb_backend_group" "bg_node_proxy_preprod" {
    description = "Backend for envoy proxies over deployed datasphere nodes (preprod)"
    folder_id   = "b1g19hobememv3hj6qsc"
    labels      = {}
    name        = "ai-node-proxy-backend-group-preprod"

    grpc {
        backend {
            name           = "ai-node-proxy-backend-group-preprod"
            port           = 443
            weight         = 100

            load_balancing_config {
                strict_locality                = false
                locality_aware_routing_percent = "100"
                mode                           = "ROUND_ROBIN"
                panic_threshold                = "40"
            }

            target_group {
                target_group_id = "ds7t88rmbvuoui3619lk"
            }
        }
    }
}
*/
