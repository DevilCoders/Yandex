resource "aws_lb" "alb" {
  name               = var.name
  internal           = var.internal
  load_balancer_type = "application"
  ip_address_type    = var.ip_address_type
  subnets            = var.subnets_ids
  security_groups    = var.security_groups_ids
}

module "target" {
  source = "../../target_group/v1"

  for_each = var.targets

  name             = "${var.name}-${each.key}"
  lb_arn           = aws_lb.alb.arn
  port             = each.value.port
  protocol         = each.value.protocol
  protocol_version = each.value.protocol_version
  certificate_arn  = var.certificate_arn
  vpc_id           = var.vpc_id
  target_type      = var.target_type
  health_check     = {
    protocol = each.value.health_check.protocol
    port     = each.value.health_check.port
    path     = each.value.health_check.path
    matcher  = each.value.health_check.matcher
  }
}
