resource "ycp_certificatemanager_certificate_request" "this" {
  # id = fpqjtqao1um3par61i4m
  lifecycle {
    prevent_destroy = true
  }

  name = "api-router-common"

  domains = [
    "api.cloud.yandex.net",
    "*.api.cloud.yandex.net",

    "api.canary.ycp.cloud.yandex.net",
    "*.api.canary.ycp.cloud.yandex.net",

    "api.ycp.cloud.yandex.net",
    "*.api.ycp.cloud.yandex.net",

    "console.cloud.yandex.ru",
    "console.cloud.yandex.com",

    "console-assessors.cloud.yandex.com",
    "console-assessors.cloud.yandex.ru",

    "console-staging.cloud.yandex.ru",

    "console.front-extprod.cloud.yandex.com",
    "console.front-extprod.cloud.yandex.ru",

    "datalens-assessors.yandex.ru",
    "back.datalens-assessors.yandex.net",
    "upload.datalens-assessors.yandex.ru",
    "staging.datalens.yandex",
  ]
  cert_provider  = "INTERNAL_CERTUM"
  challenge_type = "CHALLENGE_TYPE_UNSPECIFIED"
}
