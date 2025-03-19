locals {
  server_cert_path = "/etc/certs"

  config = {
    for zone_id, xds_endpoint in local.xds_endpoints_by_zone :
    zone_id => {
      node = {
        cluster = "ai"
        id      = "ai"
      }

      static_resources = {
        secrets = [
          {
            name = "server_cert"
            tls_certificate = {
              certificate_chain = {
                filename = "${local.server_cert_path}/servercert.pem"
              }
              private_key = {
                filename : "${local.server_cert_path}/serverkey.pem"
              }
            }
          }
        ]
        clusters = [
          {
            name            = "xds_cluster"
            connect_timeout = "5s"
            type            = "LOGICAL_DNS"
            typed_extension_protocol_options = {
              "envoy.extensions.upstreams.http.v3.HttpProtocolOptions" = {
                "@type" = "type.googleapis.com/envoy.extensions.upstreams.http.v3.HttpProtocolOptions"
                explicit_http_config = {
                  http2_protocol_options = {}
                }
              }
            }
            load_assignment = {
              cluster_name = "xds_cluster"
              endpoints = [
                {
                  lb_endpoints = [
                    {
                      endpoint = {
                        address = {
                          socket_address = {
                            address    = xds_endpoint
                            port_value = 5678
                          }
                        }
                      }
                    }
                  ]
                }
              ]
            }
          },
          {
            name                   = "ext-authz"
            connect_timeout        = "0.25s"
            type                   = "LOGICAL_DNS"
            http2_protocol_options = {}
            load_assignment = {
              cluster_name = "ext-authz"
              endpoints = [
                {
                  lb_endpoints = [
                    {
                      endpoint = {
                        address = {
                          socket_address = {
                            address    = "127.0.0.1"
                            port_value = 40002
                          }
                        }
                      }
                    }
                  ]
                }
              ]
            }
            common_lb_config = {
              healthy_panic_threshold = {
                value = 50.0
              }
            }
            health_checks = [
              {
                timeout             = "1s"
                interval            = "5s"
                interval_jitter     = "1s"
                no_traffic_interval = "5s"
                unhealthy_threshold = 1
                healthy_threshold   = 3
                grpc_health_check = {
                  service_name = "envoy.service.auth.v3.Authorization"
                }
              }
            ]
          },
          {
            name                   = "access_log_billing_service"
            connect_timeout        = "0.25s"
            type                   = "logical_dns"
            http2_protocol_options = {}
            lb_policy              = "ROUND_ROBIN"
            load_assignment = {
              cluster_name = "access_log_billing_service"
              endpoints = [
                {
                  lb_endpoints = [
                    {
                      endpoint = {
                        address = {
                          socket_address = {
                            address    = "127.0.0.1"
                            port_value = 40003
                          }
                        }
                      }
                    }
                  ]
                }
              ]
            }
          }
        ]
      }

      dynamic_resources = {
        ads_config = {
          api_type                       = "GRPC"
          transport_api_version          = "V3"
          set_node_on_first_message_only = true
          grpc_services = [
            {
              envoy_grpc = {
                cluster_name = "xds_cluster"
              }
            }
          ]
        }
        lds_config = {
          resource_api_version = "V3"
          ads                  = {}
        }
        cds_config = {
          resource_api_version = "V3"
          ads                  = {}
        }
      }

      admin = {
        access_log_path = "/dev/stdout"
        address = {
          socket_address = {
            address    = "0.0.0.0"
            port_value = 9901
          }
        }
      }
    }
  }
}
