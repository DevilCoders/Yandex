data "external" "token" {
  program = ["/bin/bash", "-c", "yc iam create-token | jq -Rn '{token: input}'"]
}

provider "ycp" {
  prod  = true
  token = data.external.token.result.token
}

# ycp_platform_alb_backend_group.bg_stt_server_general_deprecated:
resource "ycp_platform_alb_backend_group" "bg_stt_server_general_deprecated" {
  description = "STT service ALB, staging"
  folder_id   = "b1gos77c2en2ek4br48e"
  # id        = "ds7rg2v4qdj64utok8jc"
  name        = "stt-server-general-deprecated-alb"

  http {
    backend {
      name          = "stt-server-general-deprecated-backend-http"
      port          = 17002
      weight        = 99
      backend_weight = 99

      healthchecks {
        healthy_threshold   = 6
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 2

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
        target_group_id = "ds7rkr6a7shfpj0lffnd"
      }
    }

    connection {
    }
  }
}

# ycp_platform_alb_backend_group.bg_stt_server_general:
resource "ycp_platform_alb_backend_group" "bg_stt_server_general" {
  description = "STT service ALB, staging"
  folder_id   = "b1gos77c2en2ek4br48e"
  # id        = "ds7jherkpmgq9ckl792p"
  name        = "stt-server-general-alb"

  http {
    backend {
      name          = "ai-stt-server-general-rc-prod"
      port          = 17002
      weight        = 99
      backend_weight = 99

      healthchecks {
        healthcheck_port    = 17002
        healthy_threshold   = 6
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 2

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

      tcp_options {
        connection_buffer_limit_bytes = "1048576"
      }

      target_group {
        target_group_id = "ds73e67il88kho24d3km"
      }
    }

    connection {
    }
  }
}

# ycp_platform_alb_backend_group.bg_stt_server_general_rc:
resource "ycp_platform_alb_backend_group" "bg_stt_server_general_rc" {
  description = "Backend for ai-stt-server-general-rc (prod)"
  folder_id   = "b1gos77c2en2ek4br48e"
  # id        = "ds705s30vh08lr8hs8mo"
  name        = "ai-stt-server-general-rc-backend-group-prod"

  http {
    backend {
      name          = "ai-stt-server-general-rc-prod"
      port          = 17002
      weight        = 99

      healthchecks {
        healthcheck_port    = 17002
        healthy_threshold   = 6
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 2

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

      tcp_options {
        connection_buffer_limit_bytes = "1048576"
      }

      target_group {
        target_group_id = "ds73e67il88kho24d3km"
      }
    }

    connection {
    }
  }
}

# ycp_platform_alb_backend_group.bg_stt_server_kazah:
resource "ycp_platform_alb_backend_group" "bg_stt_server_kazah" {
  description = "Backend for ai-stt-server-kazah (prod)"
  folder_id   = "b1gos77c2en2ek4br48e"
  # id        = "ds705s30vh08lr8hs8mo"
  name        = "ai-stt-server-kazah-backend-group-prod"

  http {
    backend {
      name          = "ai-stt-server-kazah-prod"
      port          = 17002
      weight        = 99

      healthchecks {
        healthcheck_port    = 17002
        healthy_threshold   = 6
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 2

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

      tcp_options {
        connection_buffer_limit_bytes = "1048576"
      }

      target_group {
        target_group_id = "ds72n20umt645upnnam6"
      }
    }

    connection {
    }
  }
}

# ycp_platform_alb_backend_group.bg_stt_server_general_grpc:
resource "ycp_platform_alb_backend_group" "bg_stt_server_general_grpc" {
  description = "Backend for ai-stt-server-general grpc (prod)"
  folder_id   = "b1gndji7iaubpghf15b0"
  # id        = "ds7davt4rn9255s6enip"
  name        = "ai-stt-server-general-grpc-backend-group"

  grpc {
    backend {
      name   = "ai-stt-server-general-grpc-backend-group"
      port   = 17005
      weight = 100

      healthchecks {
        healthcheck_port    = 17005
        healthy_threshold   = 6
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 2

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
        target_group_id = "ds7rkr6a7shfpj0lffnd"
      }
    }
  }
}

# ycp_platform_alb_backend_group.bg_stt_server_general_rc_grpc:
resource "ycp_platform_alb_backend_group" "bg_stt_server_general_rc_grpc" {
  description = "Backend for ai-stt-server-general-rc grpc (prod)"
  folder_id   = "b1gndji7iaubpghf15b0"
  # id        = "ds7davt4rn9255s6enip"
  name        = "ai-stt-server-general-rc-grpc-backend-group"

  grpc {
    backend {
      name   = "ai-stt-server-general-rc-grpc-backend-group"
      port   = 17005
      weight = 100

      healthchecks {
        healthcheck_port    = 17005
        healthy_threshold   = 6
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 2

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
        target_group_id = "ds73e67il88kho24d3km"
      }
    }
  }
}

# ycp_platform_alb_backend_group.bg_stt_server_kazah_grpc:
resource "ycp_platform_alb_backend_group" "bg_stt_server_kazah_grpc" {
  description = "Backend for ai-stt-server-kazah grpc (prod)"
  folder_id   = "b1gndji7iaubpghf15b0"
  # id        = "ds7davt4rn9255s6enip"
  name        = "ai-stt-server-kazah-grpc-backend-group"

  grpc {
    backend {
      name   = "ai-stt-server-kazah-grpc-backend-group"
      port   = 17005
      weight = 100

      healthchecks {
        healthcheck_port    = 17005
        healthy_threshold   = 6
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 2

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
        target_group_id = "ds72n20umt645upnnam6"
      }
    }
  }
}

resource "ycp_platform_alb_backend_group" "bg_stt_server_general_v1_grpc" {
  description = "Backend for ai-stt-server-general-v3 grpc (prod)"
  folder_id   = "b1gos77c2en2ek4br48e"
  # id        = "ds7davt4rn9255s6enip"
  name        = "ai-stt-server-general-v3-grpc-backend-group"

  grpc {
    backend {
      name   = "ai-stt-server-general-v3-grpc-backend-group"
      port   = 17004
      weight = 100

      healthchecks {
        healthcheck_port    = 17004
        healthy_threshold   = 6
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 2

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
        target_group_id = "ds7d35aag20f57e2bsnu"
      }
    }
  }
}


resource "ycp_platform_alb_backend_group" "bg_stt_server_general_rc_v1_grpc" {
  description = "Backend for ai-stt-server-general-rc-v3 grpc (prod)"
  folder_id   = "b1gos77c2en2ek4br48e"
  # id        = "ds7davt4rn9255s6enip"
  name        = "ai-stt-server-general-rc-v3-grpc-backend-group"

  grpc {
    backend {
      name   = "ai-stt-server-general-rc-v3-grpc-backend-group"
      port   = 17004
      weight = 100
      backend_weight = 100

      healthchecks {
        healthcheck_port    = 17004
        healthy_threshold   = 6
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 2

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
        target_group_id = "ds7qhuk9fiofc9qgaqdt"
      }
    }
  }
}


resource "ycp_platform_alb_backend_group" "bg_stt_server_general_deprecated_v1_grpc" {
  description = "Backend for ai-stt-server-general-deprecated-v3 grpc (prod)"
  folder_id   = "b1gos77c2en2ek4br48e"
  # id        = "ds7davt4rn9255s6enip"
  name        = "ai-stt-server-general-deprecated-v3-grpc-backend-group"

  grpc {
    backend {
      name   = "ai-stt-server-general-deprecated-v3-grpc-backend-group"
      port   = 17004
      weight = 100

      healthchecks {
        healthcheck_port    = 17004
        healthy_threshold   = 6
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 2

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
        target_group_id = "ds7d35aag20f57e2bsnu"
      }
    }
  }
}


# ycp_platform_alb_backend_group.bg_stt_server_biovitrum:
resource "ycp_platform_alb_backend_group" "bg_stt_server_biovitrum" {
  description = "Backend for ai-stt-server-biovitrum (prod)"
  folder_id   = "b1gos77c2en2ek4br48e"
  # id        = "ds7mnkmq6g4jpimg2n2m"
  name        = "ai-stt-server-biovitrum-backend-group-prod"

  http {
    backend {
      name          = "ai-stt-server-biovitrum-prod"
      port          = 17002
      weight        = 99

      healthchecks {
        healthcheck_port    = 17002
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
        mode = "ROUND_ROBIN"
        panic_threshold = "40"
      }

      target_group {
        target_group_id = "ds73e67il88kho24d3km"
      }
    }

    connection {
    }
  }
}

# ycp_platform_alb_backend_group.bg_stt_server_hqa:
resource "ycp_platform_alb_backend_group" "bg_stt_server_hqa" {
  description = "STT service ALB hqa model"
  folder_id   = "b1gos77c2en2ek4br48e"
  # id        = "ds7omvsb9p7ol05gnosk"
  name        = "stt-server-hqa-alb"

  http {
    backend {
      name          = "stt-server-hqa-backend-http"
      port          = 17002
      weight        = 99

      healthchecks {
        healthy_threshold   = 6
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 2

        http {
          path      = "/ping"
        }
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold = "40"
      }

      target_group {
        target_group_id = "ds73e67il88kho24d3km"
      }
    }

    connection {
    }
  }
}

# ycp_platform_alb_backend_group.bg_stt_server_no_lm_remote:
resource "ycp_platform_alb_backend_group" "bg_stt_server_no_lm_remote" {
  description = "STT service ALB no_lm_remote model"
  folder_id   = "b1gos77c2en2ek4br48e"
  # id        = "ds7tvc05tqija1ev2i4g"
  name        = "stt-server-no-lm-remote-alb"

  http {
    backend {
      name          = "stt-server-no-lm-remote-backend-http"
      port          = 17002
      weight        = 99

      healthchecks {
        healthy_threshold   = 6
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 2

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
        target_group_id = "ds7j6p5u4f8a4n6vcmee"
      }
    }

    connection {
    }
  }
}

# ycp_platform_alb_backend_group.bg_stt_server_no_lm_general:
resource "ycp_platform_alb_backend_group" "bg_stt_server_no_lm_general" {
  description = "STT service ALB, staging"
  folder_id   = "b1gos77c2en2ek4br48e"
  # id        = "ds7aoggh6gbnnbf84kgv"
  name        = "stt-server-no-lm-general-alb"

  http {
    backend {
      name          = "stt-server-no-lm-general-backend-staging-http"
      port          = 17002
      weight        = 99

      healthchecks {
        healthy_threshold   = 6
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 2

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
        target_group_id = "ds7hm7s6o0a99hg7hgru"
      }
    }

    connection {
    }
  }
}

# ycp_platform_alb_backend_group.bg_stt_server_kaspi:
resource "ycp_platform_alb_backend_group" "bg_stt_server_kaspi" {
  description = "Backend for ai-stt-service-kaspi service (prod)"
  folder_id   = "b1gos77c2en2ek4br48e"
  # id        = "ds7pj5lraj7j6il26qa7"
  name        = "ai-stt-service-kaspi-server-backend-group-prod"

  http {
    backend {
      name          = "ai-stt-service-kaspi-prod"
      port          = 17002
      weight        = 99

      healthchecks {
        healthcheck_port    = 17002
        healthy_threshold   = 10
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 2

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
        target_group_id = "ds7k31dcf7ne3bnkuaeo"
      }
    }

    connection {
    }
  }
}

# ycp_platform_alb_backend_group.bg_stt_server_names:
resource "ycp_platform_alb_backend_group" "bg_stt_server_names" {
  description = "Backend for ai-stt-server-names (prod)"
  folder_id   = "b1gos77c2en2ek4br48e"
  # id        = "ds76voqbn8gkqohfintk"
  name        = "ai-stt-server-names-backend-group-prod"

  http {
    backend {
      name          = "ai-stt-server-names-prod"
      port          = 17002
      weight        = 99

      healthchecks {
        healthcheck_port    = 17002
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
        target_group_id = "ds7jol5g1ducroioihmd"
      }
    }

    connection {
    }
  }
}

# ycp_platform_alb_backend_group.bg_stt_server_rupor:
resource "ycp_platform_alb_backend_group" "bg_stt_server_rupor" {
  description = "Backend for ai-stt-server-rupor (prod)"
  folder_id   = "b1gos77c2en2ek4br48e"
  # id        = "ds7lj0pni3run8s77o0h"
  name        = "ai-stt-server-rupor-backend-group-prod"

  http {
    backend {
      name          = "ai-stt-server-rupor-prod"
      port          = 17002
      weight        = 99

      healthchecks {
        healthcheck_port    = 17002
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
        target_group_id = "ds7mruvs2jb7ag4rjl6h"
      }
    }

    connection {
    }
  }
}

# ycp_platform_alb_backend_group.bg_stt_feedback:
resource "ycp_platform_alb_backend_group" "bg_stt_feedback" {
  description = "Backend for ai-stt-feedback service (prod)"
  folder_id   = "b1g2tpck4um2iufss8gm"
  # id        = "ds7cdj3q43gou3qqm5sq"
  name        = "ai-stt-feedback-backend-group-prod"

  grpc {
    backend {
      name   = "ai-stt-feedback-prod"
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
        target_group_id = "ds7u8a8jhqit27osh5p1"
      }
    }
  }
}

# ycp_platform_alb_backend_group.bg_services_proxy_stt:
resource "ycp_platform_alb_backend_group" "bg_services_proxy_stt" {
  description = "Backend for ai-services-proxy-stt service (prod)"
  folder_id   = "b1g0jv3qc7326nfuig84"
  # id        = "ds7mss1bj25ncbcmrpgq"
  name        = "ai-services-proxy-stt-backend-group-prod"

  grpc {
    backend {
      name   = "ai-services-proxy-stt-green"
      port   = 443
      weight = 100
      backend_weight = 100

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
        target_group_id = "ds7gp9ur850c9ba41och"
      }
    }
    backend {
      name   = "ai-services-proxy-stt-blue"
      port   = 443
      weight = -1
      backend_weight = -1

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
        target_group_id = "ds7fvrqrvf50e0tirppu"
      }
    }
  }
}

# ycp_platform_alb_backend_group.bg_services_proxy_translate_rest:
resource "ycp_platform_alb_backend_group" "bg_services_proxy_translate_rest" {
  description = "Backend for ai-services-proxy-translate-rest service (prod)"
  folder_id   = "b1g0jv3qc7326nfuig84"
  # id        = "ds7nascuvlvincrqrkji"
  name        = "ai-services-proxy-translate-rest-backend-group-prod"

  http {
    backend {
      name          = "ai-services-proxy-translate-rest-prod"
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

      passive_healthcheck {
        base_ejection_time                    = "30s"
        consecutive_gateway_failure           = 2
        enforcing_consecutive_gateway_failure = 100
        interval                              = "10s"
        max_ejection_percent                  = 66
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      target_group {
        target_group_id = "ds7g7vhdv2rhh77l78fh"
      }
    }

    connection {
    }
  }
}

# ycp_platform_alb_backend_group.bg_services_proxy_translate:
resource "ycp_platform_alb_backend_group" "bg_services_proxy_translate" {
  description = "Backend for ai-services-proxy-translate service (prod)"
  folder_id   = "b1g0jv3qc7326nfuig84"
  # id        = "ds7nv2la36c8oqsvvu9e"
  name        = "ai-services-proxy-translate-backend-group-prod"

  grpc {
    backend {
      name   = "ai-services-proxy-translate-prod"
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
        target_group_id = "ds7c2eljugq7d6hndgat"
      }
    }
  }
}

# ycp_platform_alb_backend_group.bg_services_proxy_tts_rest:
resource "ycp_platform_alb_backend_group" "bg_services_proxy_tts_rest" {
  description = "Backend for ai-services-proxy-tts-rest service (prod)"
  folder_id   = "b1g0jv3qc7326nfuig84"
  # id        = "ds7pghde8g2nqfc25k86"
  name        = "ai-services-proxy-tts-rest-backend-group-prod"

  http {
    backend {
      name          = "ai-services-proxy-tts-rest-prod"
      port          = 80
      weight        = 99

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
        target_group_id = "ds73o8o97rd42mrrshn1"
      }
    }

    backend {
      name          = "ai-services-proxy-tts-rest-canary-prod"
      port          = 80
      weight        = -1

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
        target_group_id = "ds712njr1r77r41qgegu"
      }
    }

    connection {
    }
  }
}

# ycp_platform_alb_backend_group.bg_services_proxy_tts:
resource "ycp_platform_alb_backend_group" "bg_services_proxy_tts" {
  description = "Backend for ai-tts-adaptive service (prod)"
  folder_id   = "b1g0jv3qc7326nfuig84"
  # id        = "ds7pr4h9j3soraniebus"
  name        = "ai-tts-adaptive-backend-group-prod"

  grpc {
    backend {
      name   = "ai-services-proxy-tts-prod"
      port   = 443
      weight = 100

      healthchecks {
        healthcheck_port    = 443
        healthy_threshold   = 2
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 3

        grpc {
          service_name = "yandex.cloud.ai.tts.v1.TtsService"
        }
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      target_group {
        target_group_id = "ds7sii6nnvmq00428u7e"
      }
    }

    connection {
    }
  }
}

# ycp_platform_alb_backend_group.bg_services_proxy_locator:
resource "ycp_platform_alb_backend_group" "bg_services_proxy_locator" {
  description = "Backend for ai-services-proxy-locator service (prod)"
  folder_id   = "b1g0jv3qc7326nfuig84"
  # id        = "ds7qi657t1pnki8vm1lk"
  name        = "ai-services-proxy-locator-backend-group-prod"

  grpc {
    backend {
      name   = "ai-services-proxy-locator-prod"
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
        target_group_id = "ds790d3ehqem0gk4lpls"
      }
    }
  }
}

# ycp_platform_alb_backend_group.bg_services_proxy_vision:
resource "ycp_platform_alb_backend_group" "bg_services_proxy_vision" {
  description = "Backend for ai-services-proxy-vision service (prod)"
  folder_id   = "b1g0jv3qc7326nfuig84"
  # id        = "ds7rgib7lprbtvo1c3ha"
  name        = "ai-services-proxy-vision-backend-group-prod"

  grpc {
    backend {
      name   = "ai-services-proxy-vision-prod"
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
        target_group_id = "ds7op5k7v6j83orebir5"
      }
    }
  }
}

# ycp_platform_alb_backend_group.bg_services_proxy_stt_rest:
resource "ycp_platform_alb_backend_group" "bg_services_proxy_stt_rest" {
  description = "Backend for ai-services-proxy-stt-rest service (prod)"
  folder_id   = "b1g0jv3qc7326nfuig84"
  # id        = "ds7sgm5eh1bksrq0olv0"
  name        = "ai-services-proxy-stt-rest-backend-group-prod"

  http {
    backend {
      name          = "ai-services-proxy-stt-rest-prod"
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

      target_group {
        target_group_id = "ds7b5vg2rvqbre585nkd"
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      tcp_options {
        connection_buffer_limit_bytes = 32768
    }
  }

    connection {
    }
  }
}

# ycp_platform_alb_backend_group.bg_node_deployer_service_prod will be destroyed
resource "ycp_platform_alb_backend_group" "bg_node_deployer_service_prod" {
  description = "Backend for ai-node-deployer service (prod)"
  folder_id   = "b1g4ujiu1dq5hh9544r6"
  # id          = "ds7mfl1v3ave9iqvtcp0"
  labels      = {}
  name        = "ai-node-deployer-service-backend-group-prod"

  grpc {
    backend {
      name           = "ai-node-deployer-prod"
      port           = 443
      weight         = 100

      target_group {
        target_group_id = "ds7pjpug84br7mj16qqu"
      }
    }
  }
}

resource "ycp_platform_alb_backend_group" "bg_node_service_prod" {
  description = "Backend for ai-node-execution service (prod)"
  folder_id   = "b1g4ujiu1dq5hh9544r6"
  labels      = {}
  name        = "ai-node-service-backend-group-prod"

  grpc {
    backend {
      name           = "ai-node-service-prod"
      port           = 443
      weight         = 100

      target_group {
        target_group_id = "ds7n5egsr98v5kg3s5ca"
      }
    }
  }
}

# ycp_platform_alb_backend_group.bg_tts_server_alena_rc
resource "ycp_platform_alb_backend_group" "bg_tts_server_alena_rc" {
  description = "TTS service alena:rc GPU"
  folder_id   = "b1gb294dat6q3ehreoet"
  # id          = "ds7bmsra6et42jrmbrim"
  name        = "tts-service-oksana-rc-alb"

  http {
    backend {
      name          = "tts-service-oksana-rc-http"
      port          = 17004
      weight        = 100

      healthchecks {
        healthy_threshold   = 2
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 3

        http {
          path      = "/tts?text=&dry_run=1"
        }
      }

      load_balancing_config {
        strict_locality                = false
        locality_aware_routing_percent = "100"
        mode                           = "ROUND_ROBIN"
        panic_threshold                = "40"
      }

      target_group {
        target_group_id = "ds7f3t7mfph9kpma27q0"
      }
    }
    connection {
      source_ip = false
    }
  }
}

# ycp_platform_alb_backend_group.bg_tts_server_alena_rc
resource "ycp_platform_alb_backend_group" "bg_tts_server_valtz" {
    description = "TTS service valtz GPU"
    folder_id   = "b1gb294dat6q3ehreoet"
    # id          = "ds7bmsra6et42jrmbrim"
    name        = "tts-service-valtz-alb"

    http {
        backend {
            name          = "tts-valtz"
            port          = 17004
            weight        = 100

            healthchecks {
                healthy_threshold   = 2
                interval            = "2s"
                timeout             = "1s"
                unhealthy_threshold = 3

                http {
                    path      = "/tts?text=&dry_run=1"
                }
            }

            load_balancing_config {
                strict_locality                = false
                locality_aware_routing_percent = "100"
                mode                           = "ROUND_ROBIN"
                panic_threshold                = "40"
            }

            target_group {
                target_group_id = "ds7ldqe68s1qri1ptf6b"
            }
        }
        connection {
            source_ip = false
        }
    }
}

# ycp_platform_alb_backend_group.bg_tts_server_sergey
resource "ycp_platform_alb_backend_group" "bg_tts_server_sergey" {
  description = "TTS service sergey GPU"
  folder_id   = "b1gb294dat6q3ehreoet"
  # id          = "ds7kfukrcmlfg4lt3d0b"
  name        = "tts-service-sergey-alb"

  http {
    backend {
      name          = "tts-service-sergey-http"
      port          = 17004
      weight        = 100
      use_http2      = true

      healthchecks {
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

      target_group {
        target_group_id = "ds782l1020jnh517kgnb"
      }
    }
  }
}

resource "ycp_platform_alb_backend_group" "bg_tts_server_sergey_no_billing" {
  description = "TTS service sergey GPU (skip billing)"
  folder_id   = "b1gb294dat6q3ehreoet"
  name        = "tts-service-sergey-no-billing-alb"

  grpc {
    backend {
      name   = "tts-service-sergey-no-billing-grpc"
      port   = 17005
      weight = 100
      backend_weight = 100

      healthchecks {
        healthcheck_port    = 17005
        healthy_threshold   = 6
        interval            = "2s"
        timeout             = "1s"
        unhealthy_threshold = 2

        grpc {}
      }

      target_group {
        target_group_id = "ds782l1020jnh517kgnb"
      }
    }
  }
}


