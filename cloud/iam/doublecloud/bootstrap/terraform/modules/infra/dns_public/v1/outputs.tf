output "tech_iam_zone_id" {
  value = aws_route53_zone.public[var.zone_tech_iam].id
}
