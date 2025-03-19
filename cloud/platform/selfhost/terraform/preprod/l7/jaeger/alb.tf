# ycp_platform_alb_http_router.jaeger-router:
resource "ycp_platform_alb_http_router" "jaeger-router" {
  # id = "a5dho02q66j5969lp8di"

  name        = "jaeger-router-preprod"
  description = "Root router for Jaeger router instances (preprod)"
  folder_id   = "aoe6vkh56uk8rmulco19" # jaeger folder
}

# ycp_platform_alb_virtual_host.vh-test-direct-response:
resource "ycp_platform_alb_virtual_host" "vh-test-direct-response" {
  authority = [
    "test.direct.response",
  ]
  http_router_id = ycp_platform_alb_http_router.jaeger-router.id
  name           = "vh-test-direct-response"
  ports = [
    443,
  ]

  route {
    name = "main_route"

    http {
      direct_response {
        status = 200

        body {
          text = "GOOD"
        }
      }

      match {
        http_method = []

        path {
          exact_match = "/direct"
        }
      }
    }
  }
}
