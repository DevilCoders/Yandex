resource "aws_security_group" "iam_frankfurt_ingress_ssh" {
  name   = "IAM ${var.infra_name} ingress SSH frankfurt"
  vpc_id = var.vpc_id

  ingress {
    from_port   = 22
    to_port     = 22
    protocol    = "tcp"

    cidr_blocks      = ["0.0.0.0/0"]
    ipv6_cidr_blocks = ["::/0"]
  }

  // Terraform removes the default rule
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"

    cidr_blocks      = ["0.0.0.0/0"]
    ipv6_cidr_blocks = ["::/0"]
  }
}

resource "aws_security_group" "iam_frankfurt_default" {
  name   = "IAM ${var.infra_name} default frankfurt"
  vpc_id   = var.vpc_id

  ingress {
    from_port        = 0
    protocol         = "-1"
    to_port          = 0
    cidr_blocks      = [var.cidr_block]
    ipv6_cidr_blocks = [var.ipv6_cidr_block]
  }

  egress {
    from_port        = 0
    protocol         = "-1"
    to_port          = 0
    cidr_blocks      = ["0.0.0.0/0"]
    ipv6_cidr_blocks = ["::/0"]
  }
}
