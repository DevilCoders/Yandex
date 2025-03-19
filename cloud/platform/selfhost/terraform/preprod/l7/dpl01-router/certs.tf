resource "ycp_certificatemanager_certificate_request" "this" {
  # id = fd3k5b2f53tr5ui86o5l
  name = "dpl01"
  domains = [
    "*.api.cloud-preprod.yandex.net",
  ]
  cert_provider  = "INTERNAL_CA"
  challenge_type = "CHALLENGE_TYPE_UNSPECIFIED"
}
