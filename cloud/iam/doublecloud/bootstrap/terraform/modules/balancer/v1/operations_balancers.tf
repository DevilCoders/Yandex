module "iam_operations_alb" {
  source = "../../../modules/general/lb/alb/v1"

  name            = local.operations_alb_name
  internal        = false
  ip_address_type = "dualstack"
  target_type     = "ip"
  service_type    = "ClusterIP"

  vpc_id              = var.infra.regions.frankfurt.vpc
  subnets_ids         = var.infra.regions.frankfurt.public_subnets
  security_groups_ids = [
    aws_security_group.lb_ya_accessible_all_tcp.id,
  ]

  certificate_arn = module.iam_operations_cert.certificate_arn

  targets = { for k, v in local.service_ports_grpc :
    k => {
      service_name     = v.service_name
      port             = v.port
      port_name        = v.port_name
      protocol         = "HTTPS"
      protocol_version = "GRPC"
      health_check     = {
        protocol = "HTTPS"
        path     = "/grpc.health.v1.Health/Check"
        port     = null
        matcher  = "0,12"
      }
    }
  }
}

resource "aws_route53_record" "iam_operations" {
  for_each = {
    a    = "A"
    aaaa = "AAAA"
  }
  zone_id = data.aws_route53_zone.internal.id
  name    = var.fqdn_operations
  type    = each.value
  alias {
    evaluate_target_health = false
    name                   = module.iam_operations_alb.lb_dns_name
    zone_id                = module.iam_operations_alb.lb_zone_id
  }

  depends_on = [
    module.iam_operations_alb,
  ]
}

module "iam_operations_cert" {
  source                            =  "../../../modules/general/public_cert/v1"

  domain_name                       = var.fqdn_operations
  public_zone_id_for_acme_challenge = data.aws_route53_zone.internal.id
}
