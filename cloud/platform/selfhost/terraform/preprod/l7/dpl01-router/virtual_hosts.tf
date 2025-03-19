# ycp_platform_alb_virtual_host.vh-test-direct-response:
resource "ycp_platform_alb_virtual_host" "vh-test-direct-response" {
  authority = [
    "test.direct.response",
  ]
  http_router_id = "a5dn4bvq4ltqvm9kguha"
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
