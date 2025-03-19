resource ycp_platform_alb_backend_group k8sapi_l7_bg {
  name        = "k8s-api-backend-preprod"
  description = "Backend for k8s API preprod"

  grpc {
    backend {
      name   = "k8s-api-backend-preprod"
      weight = 100
      port   = 443
      target_group {
        target_group_id = ycp_microcosm_instance_group_instance_group.group.platform_l7_load_balancer_state.0.target_group_id
      }
      tls {}
      healthchecks {
        timeout             = "1s"
        interval            = "2s"
        healthy_threshold   = 2
        unhealthy_threshold = 3
        grpc {
          service_name = "grpc.health.v1.Health"
        }
      }
      passive_healthcheck {
        base_ejection_time                    = "30s"
        consecutive_gateway_failure           = 2
        enforcing_consecutive_gateway_failure = 100
        interval                              = "10s"
        max_ejection_percent                  = 66
      }
    }
  }
}

resource ycp_platform_alb_backend_group k8sapi_proxy_l7_bg {
  name        = "k8s-api-proxy-backend-preprod"
  description = "k8s api proxy for client masters"

  http {
    backend {
      name   = "k8s-api-proxy-backend-preprod"
      weight = 100
      port   = 8443
      use_http2 = true
      target_group {
        target_group_id = ycp_microcosm_instance_group_instance_group.group.platform_l7_load_balancer_state.0.target_group_id
      }
      tls {}
      passive_healthcheck {
        base_ejection_time                    = "30s"
        consecutive_gateway_failure           = 2
        enforcing_consecutive_gateway_failure = 100
        interval                              = "10s"
        max_ejection_percent                  = 66
      }
    }
  }
}
