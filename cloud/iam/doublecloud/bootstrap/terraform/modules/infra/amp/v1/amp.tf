resource "aws_prometheus_workspace" "prom" {
  alias = "iam"
}

locals {
  name = "com.amazonaws.eu-central-1.aps-workspaces"
}

resource "aws_security_group" "endpoint" {
  name   = "allow-from-and-to-managed-prometheus-endpoint"
  vpc_id = var.vpc_id

  ingress {
    protocol  = "-1"
    from_port = 0
    to_port   = 0

    description     = "unlimited access to prometheus"
    security_groups = var.security_group_ids
  }
}

resource "aws_vpc_endpoint" "endpoint" {
  vpc_id             = var.vpc_id
  service_name       = local.name
  vpc_endpoint_type  = "Interface"
  subnet_ids         = var.subnet_ids
  security_group_ids = [aws_security_group.endpoint.id]

  tags = {
    Name = local.name
  }
}
