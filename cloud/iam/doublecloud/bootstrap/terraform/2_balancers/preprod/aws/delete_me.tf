data "aws_route53_zone" "legacy_yadc_io_zone" {
  name   = module.constants.zone_private_legacy
  vpc_id = local.infra.regions.frankfurt.vpc
}

resource "aws_route53_record" "legacy_yadc_io_tls" {
  for_each = {
    a    = "A"
    aaaa = "AAAA"
  }
  zone_id = data.aws_route53_zone.legacy_yadc_io_zone.id
  name    = "iam.internal.yadc.io"
  type    = each.value
  alias {
    evaluate_target_health = false
    name                   = "iam-tls-nlb-preprod-6dac61baf11d7d66.elb.eu-central-1.amazonaws.com" // module.iam_tls_nlb.lb_dns_name
    zone_id                = "Z3F0SRJ5LGBH90"                                                      // module.iam_tls_nlb.lb_zone_id
  }
}
