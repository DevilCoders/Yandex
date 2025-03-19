locals {
    # See https://st.yandex-team.ru/CLOUD-46465
    translate_rest_backend_group_id = "ds7nascuvlvincrqrkji"

    # See https://st.yandex-team.ru/CLOUD-46464
    tts_rest_backend_group_id = "ds7pghde8g2nqfc25k86"

    # See https://st.yandex-team.ru/CLOUD-48148
    stt_streaming_backend_group_id = "ds7mss1bj25ncbcmrpgq"

    # See https://st.yandex-team.ru/CLOUD-49041
    tts_grpc_backend_group_id = "ds7pr4h9j3soraniebus"
}

# ycp_platform_alb_virtual_host.stt_v1_l7_vh ds7bg0fn5m8f3phddumq/vh-stt-api-cloud-yandex-net-prod
resource "ycp_platform_alb_virtual_host" stt_v1_l7_vh {
    http_router_id = "ds7bg0fn5m8f3phddumq" // dpl01-router
    authority = [
        "stt.api.cloud.yandex.net",
    ]
    name = "vh-stt-api-cloud-yandex-net-prod"
    ports = [
        443,
    ]

    route {
        name = "v1_stt"

        http {
            match {
                path {
                    prefix_match = "/speech/v1/stt"
                }
            }

            route {
                auto_host_rewrite = false
                backend_group_id  = ycp_platform_alb_backend_group.ai_services_proxy_stt_rest_l7_bg.id
            }
        }
    }

    route {
        name = "yandex_cloud_ai_route_stt_streaming"

        grpc {
            match {
                fqmn {
                    prefix_match = "/yandex.cloud.ai.stt.v2.SttService/StreamingRecognize"
                }
            }

            route {
                auto_host_rewrite = false
                backend_group_id  = local.stt_streaming_backend_group_id
            }
        }
    }
    route {
        name = "yandex_cloud_ai_route"
        // Fallback to Gateway for other services and methods.
        grpc {
            match {
                fqmn {
                    prefix_match = "/yandex.cloud.ai"
                }
            }

            route {
                auto_host_rewrite = false
                backend_group_id  = ycp_platform_alb_backend_group.vision_v2_l7_bg.id
            }
        }
    }
    route {
        name = "grpc_health_route"

        grpc {
            match {
                fqmn {
                    prefix_match = "/grpc.health"
                }
            }

            route {
                auto_host_rewrite = false
                backend_group_id  = local.stt_streaming_backend_group_id
            }
        }
    }
    route {
        name = "grpc_reflection_route"

        grpc {
            match {
                fqmn {
                    prefix_match = "/grpc.reflection"
                }
            }

            route {
                auto_host_rewrite = false
                backend_group_id  = ycp_platform_alb_backend_group.vision_v2_l7_bg.id
            }
        }
    }
}

# ycp_platform_alb_virtual_host.vision_v2_l7_vh ds7bg0fn5m8f3phddumq/vh-vision-api-cloud-yandex-net-prod
resource "ycp_platform_alb_virtual_host" vision_v2_l7_vh {
    http_router_id = "ds7bg0fn5m8f3phddumq" // dpl01-router
    authority = [
        "locator.api.cloud.yandex.net",
        "transcribe.api.cloud.yandex.net",
        "vision.api.cloud.yandex.net",
    ]
    name = "vh-vision-api-cloud-yandex-net-prod"
    ports = [
        443,
    ]

    route {
        name = "v1_stt"

        http {
            match {
                path {
                    prefix_match = "/speech/v1/stt"
                }
            }

            route {
                auto_host_rewrite = false
                backend_group_id  = ycp_platform_alb_backend_group.stt_private_l7_bg.id
                host_rewrite      = "stt.private-api.cloud.yandex.net"
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
                backend_group_id  = ycp_platform_alb_backend_group.vision_v2_l7_bg.id
            }
        }
    }
}

# ycp_platform_alb_virtual_host.translate_v1_l7_vh ds7bg0fn5m8f3phddumq/vh-translate-api-v1-prod
resource "ycp_platform_alb_virtual_host" translate_v1_l7_vh {
    http_router_id = "ds7bg0fn5m8f3phddumq" // dpl01-router
    authority = [
        "translate.api.cloud.yandex.net",
    ]
    name = "vh-translate-api-v1-prod"
    ports = [
        443,
    ]

    route {
        name = "main_route"

        http {

            match {
                path {
                    prefix_match = "/translate/v1/"
                }
            }

            route {
                auto_host_rewrite  = false
                backend_group_id   = local.translate_rest_backend_group_id
                support_websockets = false
            }
        }
    }
    route {
        name = "v1_to_v2_route"

        http {

            match {
                path {
                    prefix_match = "/translate/v2/"
                }
            }

            route {
                auto_host_rewrite  = false
                backend_group_id   = ycp_platform_alb_backend_group.vision_v2_l7_bg.id
                support_websockets = false
            }
        }
    }
    route {
        name = "yandex_cloud_ai_route"

        grpc {
            match {
                fqmn {
                    prefix_match = "/yandex.cloud.ai"
                }
            }

            route {
                auto_host_rewrite = false
                backend_group_id  = ycp_platform_alb_backend_group.vision_v2_l7_bg.id
            }
        }
    }
    route {
        name = "grpc_health_route"

        grpc {
            match {
                fqmn {
                    prefix_match = "/grpc.health"
                }
            }

            route {
                auto_host_rewrite = false
                backend_group_id  = ycp_platform_alb_backend_group.vision_v2_l7_bg.id
            }
        }
    }
    route {
        name = "grpc_reflection_route"

        grpc {
            match {
                fqmn {
                    prefix_match = "/grpc.reflection"
                }
            }

            route {
                auto_host_rewrite = false
                backend_group_id  = ycp_platform_alb_backend_group.vision_v2_l7_bg.id
            }
        }
    }
}

# CLOUD-46464
# ycp_platform_alb_virtual_host.tts_api_l7_vh ds7bg0fn5m8f3phddumq/vh-tts-api-cloud-prod-yandex-net
resource "ycp_platform_alb_virtual_host" tts_api_l7_vh {
  authority = [
    "tts.api.cloud.yandex.net",
  ]
  http_router_id = "ds7bg0fn5m8f3phddumq" // dpl01-router
  name           = "vh-tts-api-cloud-prod-yandex-net"
  ports = [
    443,
  ]

  route {
    name = "main_route"

    http {
      match {
        path {
          prefix_match = "/speech/v1/tts"
        }
      }

      route {
        auto_host_rewrite = false
        backend_group_id  = local.tts_rest_backend_group_id
      }
    }
  }

  route {
    name = "grpc_route"
    grpc {
        match {
            fqmn {
                prefix_match = "/"
            }
        }
        route {
            auto_host_rewrite = false
            backend_group_id  = local.tts_grpc_backend_group_id
        }
    }
  }
}
