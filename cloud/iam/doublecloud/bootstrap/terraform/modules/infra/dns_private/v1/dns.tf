resource "aws_route53_zone" "private" {
  for_each = { for zone_name in var.private_zones : zone_name => zone_name }
  name     = each.value

  vpc {
    vpc_id = var.vpc_id
  }

  comment = "iam"
}
