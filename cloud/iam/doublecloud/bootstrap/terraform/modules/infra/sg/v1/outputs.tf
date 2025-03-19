output "iam_frankfurt_ingress_ssh_sg_id" {
  value = aws_security_group.iam_frankfurt_ingress_ssh.id
}

output "iam_frankfurt_default_sg_id" {
  value = aws_security_group.iam_frankfurt_default.id
}
