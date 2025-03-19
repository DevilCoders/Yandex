locals {
    backend_port = 443

    all_backends = [
        {
            name = "ai-gateway-backend-prod"
            target_group_id = local.main_backend.group.target_group_id
            weight = local.main_backend.weight
        },{
            name = "ai-gateway-backend-prod-prev"
            target_group_id = local.prev_backend.group.target_group_id
            weight = local.prev_backend.weight
        }
    ]

    backends = [for v in local.all_backends: v if v.weight >= 0 ]
}

# ycp_platform_alb_backend_group.vision_v2_l7_bg ds75ppp1i0rkr533m1fc
resource "ycp_platform_alb_backend_group" vision_v2_l7_bg {
    # id        = "ds75ppp1i0rkr533m1fc"
    description = "Backend for vision-api-cloud-yandex-net service (prod)"
    folder_id   = var.yc_folder
    labels      = {}
    name        = "vision-api-cloud-yandex-net-backend-group-prod"

    grpc {
        dynamic backend {
            for_each = (local.backends)
            content {
                name   = backend.value.name
                port   = local.backend_port
                weight = backend.value.weight

                healthchecks {
                    healthy_threshold = 2
                    interval = "1s"
                    timeout = "0.100s"
                    unhealthy_threshold = 3

                    grpc {}
                }

                passive_healthcheck {
                    max_ejection_percent = 66
                    consecutive_gateway_failure = 2
                    enforcing_consecutive_gateway_failure = 100
                    interval = "10s"
                    base_ejection_time = "30s"
                }

                http2_options {
                    initial_connection_window_size = 268435456
                    # 256Mb
                    initial_stream_window_size = 268435456
                    # 256Mb
                    max_concurrent_streams = 10
                }

                tcp_options {
                    connection_buffer_limit_bytes = 32768
                    # 32Kb
                }

                target_group {
                    target_group_id  = backend.value.target_group_id
                }

                tls {}
            }
        }
        connection {}
    }
}

# ycp_platform_alb_backend_group.translate_private_api_l7_bg ds7cn3so4tuj1q3769bm
resource "ycp_platform_alb_backend_group" translate_private_api_l7_bg {
    # id          = "ds7cn3so4tuj1q3769bm"
    description = "Backend for translate-api-v1 service (prod)"
    folder_id   = var.yc_folder
    labels      = {}
    name        = "translate-api-v1-backend-group-prod"

    http {
        backend {
            allow_connect = false
            name          = "translate_api_v1"
            port          = 443
            use_http2     = true
            weight        = 100

            passive_healthcheck {
                max_ejection_percent                  = 66
                consecutive_gateway_failure           = 2
                enforcing_consecutive_gateway_failure = 100
                interval                              = "10s"
                base_ejection_time                    = "30s"
            }

            target {
                endpoint {
                    hostname = "translate.private-api.cloud.yandex.net"
                }
            }

            tls {
                sni = "translate.private-api.cloud.yandex.net"
            }
        }

        connection {}
    }
}

# ycp_platform_alb_backend_group.stt_private_l7_bg ds7o503c4tc4nvjo2apj
resource "ycp_platform_alb_backend_group" stt_private_l7_bg {
    # id          = "ds7o503c4tc4nvjo2apj"
    description = "Backend for stt-v1 service (prod)"
    folder_id   = var.yc_folder
    labels      = {}
    name        = "stt-api-v1-backend-group-prod"

    http {
        backend {
            allow_connect = false
            name          = "stt_api_v1"
            port          = 443
            use_http2     = true
            weight        = 100

            passive_healthcheck {
                max_ejection_percent                  = 66
                consecutive_gateway_failure           = 2
                enforcing_consecutive_gateway_failure = 100
                interval                              = "10s"
                base_ejection_time                    = "30s"
            }

            target {
                endpoint {
                    hostname = "stt.private-api.cloud.yandex.net"
                }
            }

            tls {
                sni = "stt.private-api.cloud.yandex.net"
            }
        }

        connection {}
    }
}

# ycp_platform_alb_backend_group.ai_services_proxy_stt_rest_l7_bg ds7sgm5eh1bksrq0olv0
resource "ycp_platform_alb_backend_group" "ai_services_proxy_stt_rest_l7_bg" {
    # id          = "ds7sgm5eh1bksrq0olv0"
    description = "Backend for ai-services-proxy-stt-rest service (prod)"
    folder_id   = "b1g0jv3qc7326nfuig84"
    labels      = {}
    name        = "ai-services-proxy-stt-rest-backend-group-prod"

    http {
        backend {
            allow_connect = false
            name          = "ai-services-proxy-stt-rest-prod"
            port          = 80
            use_http2     = false
            weight        = 100

            healthchecks {
                healthcheck_port    = 80
                healthy_threshold   = 2
                interval            = "2s"
                timeout             = "0.500s"
                unhealthy_threshold = 3

                http {
                    path      = "/ping"
                    use_http2 = false
                }
            }

            passive_healthcheck {
                base_ejection_time                    = "30s"
                consecutive_gateway_failure           = 2
                enforcing_consecutive_gateway_failure = 100
                interval                              = "10s"
                max_ejection_percent                  = 66
            }

            target_group {
                target_group_id = "ds7b5vg2rvqbre585nkd"
            }
        }

        connection {
            source_ip = false
        }
    }
}
