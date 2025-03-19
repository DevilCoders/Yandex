module "iam_nlb" {
  source = "../../../modules/general/lb/nlb/v1"

  name             = "iam-nlb-${var.infra.name}"
  vpc_id           = var.infra.regions.frankfurt.vpc
  subnets_ids      = var.infra.regions.frankfurt.private_subnets
  internal         = true
  ip_address_type  = "ipv4"
  protocol         = "TCP"
  target_type      = "ip"
  service_type     = "ClusterIP"

  targets = { for k, v in merge(local.service_ports_grpc, local.service_ports_https) :
    k => {
      service_name = v.service_name
      port         = v.port
      port_name    = v.port_name
      alpn_policy  = null
      health_check = local.health_check_tcp
    }
  }
}

module "iam_tls_nlb" {
  source = "../../../modules/general/lb/nlb/v1"

  name             = "iam-tls-nlb-${var.infra.name}"
  vpc_id           = var.infra.regions.frankfurt.vpc
  subnets_ids      = var.infra.regions.frankfurt.private_subnets
  internal         = true
  ip_address_type  = "ipv4"
  protocol         = "TLS"
  target_type      = "ip"
  service_type     = "ClusterIP"

  certificate_arn = module.iam_internal_cert.certificate_arn

  targets    = merge(
    { for k, v in local.service_ports_grpc :
      k => {
        service_name = v.service_name
        port         = v.port
        port_name    = v.port_name
        alpn_policy  = "HTTP2Only"
        health_check = local.health_check_tcp
      }
    },
    { for k, v in local.service_ports_https :
      k => {
        service_name = v.service_name
        port         = v.port
        port_name    = v.port_name
        alpn_policy  = null
        health_check = local.health_check_tcp
      }
    }
  )
}

resource "aws_lb" "iam_public_alb" {
  name               = local.iam_public_alb_name
  internal           = false
  load_balancer_type = "application"
  ip_address_type    = "dualstack"
  subnets            = var.infra.regions.frankfurt.public_subnets
  security_groups    = [
    var.iam_default_security_group_id,
    aws_security_group.iam_public_alb.id,
  ]
}

resource "aws_lb_target_group" "oauth" {
  name             = "${local.iam_public_alb_name}-oauth"
  port             = local.service_ports_https.oauth.port
  protocol         = "HTTPS"
  protocol_version = "HTTP1"
  vpc_id           = var.infra.regions.frankfurt.vpc
  target_type      = "ip"
  health_check {
    protocol = "HTTPS"
    path     = "/ping"
  }
}

resource "aws_lb_target_group" "auth_ui" {
  name             = "${local.iam_public_alb_name}-auth-ui"
  port             = local.service_ports_others.auth_ui.port
  protocol         = "HTTP"
  protocol_version = "HTTP1"
  vpc_id           = var.infra.regions.frankfurt.vpc
  target_type      = "ip"
  health_check {
    protocol = "HTTP"
    path     = "/ping"
  }
}

resource "aws_lb_listener_rule" "iam_public_alb_logout" {
  listener_arn = aws_lb_listener.iam_public_alb.arn
  priority     = 100

  condition {
    path_pattern {
      values = ["/federations/logout"]
    }
  }

  action {
    type             = "forward"
    target_group_arn = aws_lb_target_group.auth_ui.arn
  }

  depends_on = [
    aws_lb_listener.iam_public_alb,
    aws_lb_target_group.auth_ui,
  ]
}

resource "aws_lb_listener_rule" "iam_public_alb_oauth" {
  listener_arn = aws_lb_listener.iam_public_alb.arn
  priority     = 110

  condition {
    path_pattern {
      values = [
        "/federations/*",
        "/oauth/*",
      ]
    }
  }

  action {
    type             = "forward"
    target_group_arn = aws_lb_target_group.oauth.arn
  }

  depends_on = [
    aws_lb_listener.iam_public_alb,
    aws_lb_target_group.oauth,
  ]
}

resource "aws_lb_listener" "iam_public_alb" {
  load_balancer_arn = aws_lb.iam_public_alb.arn
  port              = 443
  protocol          = "HTTPS"
  certificate_arn   = module.iam_auth_cert.certificate_arn

  default_action {
    type             = "forward"
    target_group_arn = aws_lb_target_group.auth_ui.arn
  }

  depends_on = [
    aws_lb_target_group.auth_ui
  ]
}

resource "aws_lb_listener" "redirect_to_ssl" {
  load_balancer_arn = aws_lb.iam_public_alb.arn
  port              = 80
  protocol          = "HTTP"

  default_action {
    type = "redirect"

    redirect {
      port        = 443
      protocol    = "HTTPS"
      status_code = "HTTP_301"
    }
  }
}
