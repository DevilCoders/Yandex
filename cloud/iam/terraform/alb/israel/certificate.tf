resource "ycp_certificatemanager_certificate_request" "auth" {
  lifecycle {
    prevent_destroy = true
  }

  name           = "auth"
  folder_id      = local.openid_folder.id
  description    = "Used by auth service (israel)"
  domains        = [
    local.auth_fqdn
  ]
  cert_provider  = "LETS_ENCRYPT"
  challenge_type = "DNS"
}
