locals {
    backend_port = 443

    healthchecks_healthy_threshold   = 2
    healthchecks_interval            = "1s"
    healthchecks_timeout             = "0.160s"
    healthchecks_unhealthy_threshold = 3

    passive_healthcheck_base_ejection_time                    = "30s"
    passive_healthcheck_consecutive_gateway_failure           = 2
    passive_healthcheck_enforcing_consecutive_gateway_failure = 100
    passive_healthcheck_interval                              = "10s"
    passive_healthcheck_max_ejection_percent                  = 66

    all_backends = [
        {
            name = "api-backend-preprod"
            target_group_id = local.main_backend.group.target_group_id
            weight = local.main_backend.weight
        },{
            name = "api-backend-preprod-prev"
            target_group_id = local.prev_backend.group.target_group_id
            weight = local.prev_backend.weight
        }
    ]

    backends = [for v in local.all_backends: v if v.weight >= 0 ]
}

# ycp_platform_alb_backend_group.gateway_l7_bg a5das09cgc7i8k0renf8
resource "ycp_platform_alb_backend_group" gateway_l7_bg {
    name        = "api-backend-group-preprod"
    description = "Backend for api service (preprod)"
    folder_id   = var.yc_folder

    grpc {
        dynamic backend {
            for_each = (local.backends)
            content {
                name   = backend.value.name
                port   = local.backend_port
                weight = backend.value.weight

                healthchecks {
                    healthy_threshold   = local.healthchecks_healthy_threshold
                    interval            = local.healthchecks_interval
                    timeout             = local.healthchecks_timeout
                    unhealthy_threshold = local.healthchecks_unhealthy_threshold

                    grpc {}
                }

                passive_healthcheck {
                    base_ejection_time                    = local.passive_healthcheck_base_ejection_time
                    consecutive_gateway_failure           = local.passive_healthcheck_consecutive_gateway_failure
                    enforcing_consecutive_gateway_failure = local.passive_healthcheck_enforcing_consecutive_gateway_failure
                    interval                              = local.passive_healthcheck_interval
                    max_ejection_percent                  = local.passive_healthcheck_max_ejection_percent
                }

                target_group {
                    target_group_id = backend.value.target_group_id
                }

                tls {}
            }
        }

        connection {}
    }
}

# ycp_platform_alb_backend_group.gateway_ycp_l7_bg a5d8a51t8o6afrvdsigj
resource "ycp_platform_alb_backend_group" gateway_ycp_l7_bg {
    name        = "api-ycp-backend-group-preprod"
    description = "Backend for api service (preprod)"
    folder_id   = var.yc_folder

    grpc {
        backend {
            name   = "api-ycp-backend-preprod"
            port   = local.backend_port
            weight = 100

            healthchecks {
                healthy_threshold   = local.healthchecks_healthy_threshold
                interval            = local.healthchecks_interval
                timeout             = local.healthchecks_timeout
                unhealthy_threshold = local.healthchecks_unhealthy_threshold

                grpc {}
            }

            passive_healthcheck {
                base_ejection_time                    = local.passive_healthcheck_base_ejection_time
                consecutive_gateway_failure           = local.passive_healthcheck_consecutive_gateway_failure
                enforcing_consecutive_gateway_failure = local.passive_healthcheck_enforcing_consecutive_gateway_failure
                interval                              = local.passive_healthcheck_interval
                max_ejection_percent                  = local.passive_healthcheck_max_ejection_percent
            }

            target_group {
                target_group_id = local.prev_backend.group.target_group_id
            }

            tls {}
        }

        connection {}
    }
}

# ycp_platform_alb_backend_group.gateway_canary_l7_bg a5d94ag15u9bhgl9fkkb
resource "ycp_platform_alb_backend_group" gateway_canary_l7_bg {
    name        = "api-canary-backend-group-preprod"
    description = "Backend for api-canary service (preprod)"
    folder_id   = var.yc_folder

    grpc {
        backend {
            name   = "api-canary-backend-preprod"
            port   = local.backend_port
            weight = 100

            healthchecks {
                healthy_threshold   = local.healthchecks_healthy_threshold
                interval            = local.healthchecks_interval
                timeout             = local.healthchecks_timeout
                unhealthy_threshold = local.healthchecks_unhealthy_threshold

                grpc {}
            }

            passive_healthcheck {
                base_ejection_time                    = local.passive_healthcheck_base_ejection_time
                consecutive_gateway_failure           = local.passive_healthcheck_consecutive_gateway_failure
                enforcing_consecutive_gateway_failure = local.passive_healthcheck_enforcing_consecutive_gateway_failure
                interval                              = local.passive_healthcheck_interval
                max_ejection_percent                  = local.passive_healthcheck_max_ejection_percent
            }

            target_group {
                target_group_id = local.canary_backend.group.target_group_id
            }

            tls {}
        }

        connection {}
    }
}
