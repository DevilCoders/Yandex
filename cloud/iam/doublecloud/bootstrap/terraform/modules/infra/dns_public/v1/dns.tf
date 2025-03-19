resource "aws_route53_delegation_set" "team_internet" {
  reference_name = var.delegation_set_reference_name
}

resource "aws_route53_zone" "public" {
  for_each          = { for zone_name in var.public_zones: zone_name => zone_name }
  name              = each.value
  delegation_set_id = aws_route53_delegation_set.team_internet.id
}
