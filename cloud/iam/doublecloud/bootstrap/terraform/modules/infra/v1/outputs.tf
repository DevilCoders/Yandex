output "iam_frankfurt_ingress_ssh_sg_id" {
  value = module.security_groups.iam_frankfurt_ingress_ssh_sg_id
}

output "iam_frankfurt_a_public_subnet_id" {
  value = aws_subnet.iam_frankfurt_a_public.id
}

output "iam_frankfurt_b_public_subnet_id" {
  value = aws_subnet.iam_frankfurt_b_public.id
}

output "iam_frankfurt_c_public_subnet_id" {
  value = aws_subnet.iam_frankfurt_c_public.id
}
