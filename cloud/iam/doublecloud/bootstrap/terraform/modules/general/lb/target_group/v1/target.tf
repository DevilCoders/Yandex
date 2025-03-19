resource "aws_lb_target_group" "target_group" {
  name             = var.name
  port             = var.port
  protocol         = var.protocol
  protocol_version = var.protocol_version
  vpc_id           = var.vpc_id
  target_type      = var.target_type
  health_check {
    protocol = var.health_check.protocol
    port     = var.health_check.port
    path     = var.health_check.path
    matcher  = var.health_check.matcher
  }
}

resource "aws_lb_listener" "listener" {
  load_balancer_arn = var.lb_arn
  port              = var.port
  protocol          = var.protocol
  certificate_arn   = var.certificate_arn
  alpn_policy       = var.alpn_policy

  default_action {
    type             = "forward"
    target_group_arn = aws_lb_target_group.target_group.arn
  }
}
