# Until CLOUD-87923, if you need to add another domain, you have to create new cert resource.
resource "ycp_certificatemanager_certificate_request" "this" {
  # id = "fd3mqkcmdffr7bp3hul3"
  name = "staging"
  domains = [
    "*.l7-staging.ycp.cloud-preprod.yandex.net",
    "l7-staging.ycp.cloud-preprod.yandex.net",
  ]
  cert_provider  = "INTERNAL_CA"
  challenge_type = "CHALLENGE_TYPE_UNSPECIFIED"
}
