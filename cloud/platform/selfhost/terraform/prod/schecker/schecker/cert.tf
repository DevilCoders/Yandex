resource "ycp_certificatemanager_certificate_request" "schecker-api" {
  name = "schecker-api"
  domains = [
    "api.schecker.cloud.yandex.net",
    "schecker.private-api.cloud.yandex.net",
  ]
  cert_provider  = "INTERNAL_CA"
  challenge_type = "CHALLENGE_TYPE_UNSPECIFIED"
}
