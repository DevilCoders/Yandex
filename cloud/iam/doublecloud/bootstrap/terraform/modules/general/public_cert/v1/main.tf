resource "aws_acm_certificate" "this" {
  domain_name       = var.domain_name
  validation_method = "DNS"

  subject_alternative_names = var.subject_alternative_names
}

locals {
  // Dirty hack. Can be used due to we have no SANs in requested certificate. In opposite case we will got multiple opts.
  // If we try to parametrize `aws_route53_record` with `domain_validation_options`,
  // terraform will not be able to make plan before `aws_acm_certificate` will be created due to undefined number of parameters
  single_cert_validation_opt = {
  for dvo in tolist(aws_acm_certificate.this.domain_validation_options) : dvo.domain_name => dvo
  }[var.domain_name]
}

resource "aws_route53_record" "this" {
  zone_id = var.public_zone_id_for_acme_challenge

  name    = local.single_cert_validation_opt.resource_record_name
  type    = local.single_cert_validation_opt.resource_record_type
  ttl     = 60
  records = [local.single_cert_validation_opt.resource_record_value]
}

resource "aws_acm_certificate_validation" "this" {
  certificate_arn         = aws_acm_certificate.this.arn
  validation_record_fqdns = [
  for dvo in tolist(aws_acm_certificate.this.domain_validation_options) : substr(dvo.resource_record_name, 0, length(dvo.resource_record_name) - 1)
  ]
}
