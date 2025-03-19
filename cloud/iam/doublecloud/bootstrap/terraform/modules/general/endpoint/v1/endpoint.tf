resource "aws_vpc_endpoint" "endpoint" {
  service_name       = var.service_name
  vpc_id             = var.vpc_id
  vpc_endpoint_type  = "Interface"
  subnet_ids         = var.subnet_ids
  security_group_ids = var.security_group_ids

  tags = {
    Name = var.tag_name
  }
}

resource "aws_route53_record" "endpoint_fqdn" {
  for_each = {
    a    = "A"
    aaaa = "AAAA"
  }

  zone_id = var.route53_record_zone_id
  name    = var.route53_record_fqdn
  type    = each.value

  alias {
    name                   = aws_vpc_endpoint.endpoint.dns_entry[0].dns_name
    zone_id                = aws_vpc_endpoint.endpoint.dns_entry[0].hosted_zone_id
    evaluate_target_health = true
  }

  depends_on = [
    aws_vpc_endpoint.endpoint
  ]
}
