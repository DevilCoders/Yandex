resource "ycp_certificatemanager_certificate_request" "auth" {
  name           = "auth"
  folder_id      = local.openid_folder.id
  description    = "Used by auth service (hw-blue-lab)"
  domains        = [
    local.auth_fqdn
  ]
  cert_provider  = "INTERNAL_CA"
  challenge_type = "HTTP"
}
