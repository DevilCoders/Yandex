resource "ycp_certificatemanager_certificate_request" "this" {
  # id = fpqe1c9bkgpmjfcsf4dc
    lifecycle {
    prevent_destroy = true
  }

  name = "dpl01-router-common"

  domains = [
    "api.cloud.yandex.net",
    "*.api.cloud.yandex.net",
  ]
  cert_provider  = "INTERNAL_CERTUM"
  challenge_type = "CHALLENGE_TYPE_UNSPECIFIED"
}
