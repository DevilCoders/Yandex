resource "aws_security_group" "iam_public_alb" {
  vpc_id      = var.infra.regions.frankfurt.vpc
  name        = "IAM ${var.infra.name} public ALB"
  description = "Security group for DataCloud IAM alb"

  dynamic "ingress" {
    for_each = toset([80, 443])
    content {
      protocol  = "tcp"
      from_port = ingress.value
      to_port   = ingress.value

      description = "allow_all"

      cidr_blocks      = ["0.0.0.0/0"]
      ipv6_cidr_blocks = ["::/0"]
    }
  }
}

resource "aws_security_group" "lb_ya_accessible_all_tcp" {
  vpc_id      = var.infra.regions.frankfurt.vpc
  name        = "IAM ${var.infra.name} from Yandex nets"
  description = "Security group for LB accessible from Yandex (all TCP ports)"

  ingress {
    description      = "IAM GRPC access from _YANDEXNETS_"
    cidr_blocks      = local.yandexnets.ipv4
    ipv6_cidr_blocks = local.yandexnets.ipv6
    protocol         = "tcp"
    from_port        = min(local.service_ports_grpc_values...)
    to_port          = max(local.service_ports_grpc_values...)
  }

  // Terraform removes the default rule
  egress {
    cidr_blocks = ["0.0.0.0/0"]
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
  }
}
