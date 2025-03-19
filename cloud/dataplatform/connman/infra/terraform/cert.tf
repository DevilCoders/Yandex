resource "ycp_certificatemanager_certificate_request" "cm-certificate" {
  cert_provider  = "INTERNAL_CA"
  challenge_type = "CHALLENGE_TYPE_UNSPECIFIED"
  domains        = module.vars.domains
  name           = "cm-certificate"
}
