resource "ycp_certificatemanager_certificate_request" "this" {
  # id = fd33v1q96kc5sk3mv9p2
  name = "api-router"
  domains = [
    "api.cloud-preprod.yandex.net",
    "*.api.cloud-preprod.yandex.net",

    "api.canary.ycp.cloud-preprod.yandex.net",
    "*.api.ycp.cloud-preprod.yandex.net",

    "api.ycp.cloud-preprod.yandex.net",
    "*.api.canary.ycp.cloud-preprod.yandex.net",


    "dataproc-ui.cloud-preprod.yandex.net",
    "*.dataproc-ui.cloud-preprod.yandex.net",

    "console-preprod.cloud.yandex.com",
    "console-preprod.cloud.yandex.ru",
    "console.front-preprod.cloud.yandex.com",
    "console.front-preprod.cloud.yandex.ru",
  ]
  cert_provider  = "INTERNAL_CA"
  challenge_type = "CHALLENGE_TYPE_UNSPECIFIED"
}
