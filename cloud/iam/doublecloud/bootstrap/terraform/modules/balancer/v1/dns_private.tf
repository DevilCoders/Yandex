data "aws_route53_zone" "private" {
  name   = var.zone_private_iam
  vpc_id = var.infra.regions.frankfurt.vpc
}

resource "aws_route53_record" "iam_internal_fqdn" {
  zone_id = data.aws_route53_zone.private.id
  name    = var.fqdn_internal_iam
  type    = "A"

  alias {
    name                   = module.iam_tls_nlb.lb_dns_name
    zone_id                = module.iam_tls_nlb.lb_zone_id
    evaluate_target_health = true
  }

  depends_on = [
    data.aws_route53_zone.private,
    module.iam_tls_nlb,
  ]
}

resource "aws_route53_record" "iam_private_fqdn" {
  for_each = toset(local.service_aliases)

  zone_id = data.aws_route53_zone.private.id
  name    = "${each.key}.${var.region_suffix}.${var.zone_private_iam}"
  type    = "A"

  alias {
    name                   = module.iam_nlb.lb_dns_name
    zone_id                = module.iam_nlb.lb_zone_id
    evaluate_target_health = true
  }

  depends_on = [
    data.aws_route53_zone.private,
    module.iam_nlb
  ]
}
