resource "aws_route53_delegation_set" "team_internet" {
  reference_name = local.delegation_set_reference_name
}

resource "aws_route53_zone" "public" {
  for_each          = { for zone_name in local.public_zones: zone_name => zone_name }
  name              = each.value
  delegation_set_id = aws_route53_delegation_set.team_internet.id
}

data "aws_route53_zone" "legacy_yadc_io_zone" {
  name         = local.zone_internal_iam
  private_zone = false
}

resource "aws_route53_record" "legacy_record_for_certificate_validation" {
  zone_id = data.aws_route53_zone.legacy_yadc_io_zone.id

  name    = "_443b4cbd0397f30fb2bbbaa177dbfe67.iam.internal.yadc.io."             //local.single_cert_validation_opt.resource_record_name
  type    = "CNAME"                                                               //local.single_cert_validation_opt.resource_record_type
  ttl     = 60
  records = ["_8fc761c11701f6305b548eb7a9ef4d95.jhztdrwbnw.acm-validations.aws."] // [local.single_cert_validation_opt.resource_record_value]
}
