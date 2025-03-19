output "sa_name" {
  value = var.sa_name
}

output "namespace" {
  value = var.namespace
}

output "role_arn" {
  value = aws_iam_role.sa.arn
}
