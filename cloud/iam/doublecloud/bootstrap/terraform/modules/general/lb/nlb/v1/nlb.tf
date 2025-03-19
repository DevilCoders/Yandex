resource "aws_lb" "nlb" {
  name               = var.name
  internal           = var.internal
  load_balancer_type = "network"
  ip_address_type    = var.ip_address_type
  subnets            = var.subnets_ids

  enable_cross_zone_load_balancing = true
}

module "target" {
  source = "../../target_group/v1"

  for_each = var.targets

  name         = "${var.name}-${each.key}"
  lb_arn       = aws_lb.nlb.arn
  protocol     = var.protocol
  port         = tonumber(each.value.port)
  vpc_id       = var.vpc_id
  target_type  = var.target_type

  certificate_arn = var.certificate_arn
  alpn_policy     = each.value.alpn_policy

  health_check = {
    protocol = each.value.health_check.protocol
    port     = each.value.health_check.port
    path     = each.value.health_check.path
    matcher  = each.value.health_check.matcher
  }
}
