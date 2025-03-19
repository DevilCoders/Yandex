module "dns_private" {
  source = "../dns_private/v1"

  vpc_id                        = aws_vpc.vpc_iam_frankfurt.id
  private_zones                 = var.private_zones
}

module "dns_public" {
  source = "../dns_public/v1"

  count = var.skip_dns_public_resources ? 0 : 1

  public_zones                  = var.public_zones
  delegation_set_reference_name = var.delegation_set_reference_name
  zone_tech_iam                 = var.zone_tech_iam
}
