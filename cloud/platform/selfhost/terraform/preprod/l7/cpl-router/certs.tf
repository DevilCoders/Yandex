# Until CLOUD-87923, if you need to add another domain, you have to create new cert resource.
resource "ycp_certificatemanager_certificate_request" "this" {
  # id = fd3ce46e1j4pm5p34qhj
  name = "cpl-router"
  domains = [
    "*.private-api.ycp.cloud-preprod.yandex.net",
    "cpl.ycp.cloud-preprod.yandex.net",
    "cloud-backoffice-preprod.cloud.yandex.ru",
    "backoffice-preprod.cloud.yandex.ru",
    "*.front-preprod.cloud.yandex.ru",
  ]
  cert_provider  = "INTERNAL_CA"
  challenge_type = "CHALLENGE_TYPE_UNSPECIFIED"
}
