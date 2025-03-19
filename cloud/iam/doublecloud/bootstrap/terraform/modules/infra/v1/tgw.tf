resource "aws_ec2_transit_gateway_vpc_attachment" "tgw_attachment" {
  for_each = { for tg in var.transit_gateways : tg.transit_gateway_id => tg }

  transit_gateway_id = each.key
  vpc_id             = aws_vpc.vpc_iam_frankfurt.id
  subnet_ids         = local.private_subnets_ids

  dns_support                                     = "disable"
  ipv6_support                                    = "enable"
  transit_gateway_default_route_table_association = true
  transit_gateway_default_route_table_propagation = true
}
