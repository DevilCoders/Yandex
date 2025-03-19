# ycp_platform_alb_virtual_host.gateway_l7_vh a5d48mbh1i5o8dudgs0v/vh-api-preprod
resource "ycp_platform_alb_virtual_host" gateway_l7_vh {
    authority = [
        "*.api.cloud-preprod.yandex.net",
        "api.cloud-preprod.yandex.net",
    ]
    http_router_id = "a5d48mbh1i5o8dudgs0v"
    name           = "vh-api-preprod"
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
                backend_group_id  = ycp_platform_alb_backend_group.gateway_l7_bg.id
            }
        }
    }
}

# Secondary host for gateway. Used only for e2e tests.
# ycp_platform_alb_virtual_host.gateway_ycp_l7_vh a5d48mbh1i5o8dudgs0v/vh-api-ycp-preprod
resource "ycp_platform_alb_virtual_host" gateway_ycp_l7_vh {
    authority = [
        "*.api.ycp.cloud-preprod.yandex.net",
        "api.ycp.cloud-preprod.yandex.net",
    ]
    http_router_id = "a5d48mbh1i5o8dudgs0v"
    name           = "vh-api-ycp-preprod"
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
                backend_group_id  = ycp_platform_alb_backend_group.gateway_ycp_l7_bg.id
            }
        }
    }
}

# ycp_platform_alb_virtual_host.gateway_canary_l7_vh a5d48mbh1i5o8dudgs0v/vh-api-canary-preprod
resource "ycp_platform_alb_virtual_host" gateway_canary_l7_vh {
    authority = [
        "*.api.canary.ycp.cloud-preprod.yandex.net",
        "api.canary.ycp.cloud-preprod.yandex.net",
    ]
    http_router_id = "a5d48mbh1i5o8dudgs0v"
    name           = "vh-api-canary-preprod"
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
                backend_group_id  = ycp_platform_alb_backend_group.gateway_canary_l7_bg.id
            }
        }
    }
}
