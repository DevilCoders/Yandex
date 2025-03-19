# See https://clubs.at.yandex-team.ru/ycp/4189
# and https://storage.cloud-preprod.yandex.net/terraform/docs/latest/resources/certificatemanager_certificate_request.html
resource "ycp_certificatemanager_certificate_request" "api" {
  description = "Certificate for ${var.api_domain}"
  name = "mr-prober-api-certificate"

  domains = [
    var.api_domain
  ]

  folder_id = var.folder_id

  cert_provider = "INTERNAL_CA"

  challenge_type = "CHALLENGE_TYPE_UNSPECIFIED"
}