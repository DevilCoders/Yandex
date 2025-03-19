data "aws_route53_zone" "public" {
  name         = var.zone_public_auth
  private_zone = false
}

resource "aws_route53_record" "iam_public_lb_alias" {
  for_each = {
    a    = "A"
    aaaa = "AAAA"
  }
  zone_id = data.aws_route53_zone.public.id
  name    = var.fqdn_public_auth
  type    = each.value
  alias {
    evaluate_target_health = false
    name                   = aws_lb.iam_public_alb.dns_name
    zone_id                = aws_lb.iam_public_alb.zone_id
  }

  depends_on = [ aws_lb.iam_public_alb ]
}

module "iam_auth_cert" {
  source                            = "../../../modules/general/public_cert/v1"

  domain_name                       = var.fqdn_public_auth
  public_zone_id_for_acme_challenge = data.aws_route53_zone.public.id
}

data "aws_route53_zone" "internal" {
  name         = var.zone_internal_iam
  private_zone = false
}

resource "aws_route53_record" "iam_nlb_tls" {
  for_each = {
    a    = "A"
    aaaa = "AAAA"
  }
  zone_id = data.aws_route53_zone.internal.id
  name    = var.fqdn_internal_iam
  type    = each.value
  alias {
    evaluate_target_health = false
    name                   = module.iam_tls_nlb.lb_dns_name
    zone_id                = module.iam_tls_nlb.lb_zone_id
  }

  depends_on = [ module.iam_tls_nlb ]
}

module "iam_internal_cert" {
  source                            =  "../../../modules/general/public_cert/v1"

  domain_name                       = var.fqdn_internal_iam
  public_zone_id_for_acme_challenge = data.aws_route53_zone.internal.id

  subject_alternative_names         = var.fqdn_internal_iam_alternatives
}
