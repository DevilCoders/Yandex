# NOTIFY
data "aws_route53_zone" "notify_route53_zone" {
  name   = var.zone_notify
  vpc_id = var.infra.regions.frankfurt.vpc
}

module "endpoint_notify" {
  source = "../../../modules/general/endpoint/v1"

  service_name           = var.notify_endpoint_service_name
  tag_name               = "IAM to Notify endpoint"
  vpc_id                 = var.infra.regions.frankfurt.vpc
  subnet_ids             = var.infra.regions.frankfurt.private_subnets
  security_group_ids = [
    var.iam_default_security_group_id,
  ]

  route53_record_zone_id = data.aws_route53_zone.notify_route53_zone.id
  route53_record_fqdn    = var.fqdn_notify
}


# BILLING
data "aws_route53_zone" "billing_route53_zone" {
  name   = var.zone_private_iam
  vpc_id = var.infra.regions.frankfurt.vpc
}

module "endpoint_billing" {
  source = "../../../modules/general/endpoint/v1"

  service_name       = var.billing_endpoint_service_name
  tag_name           = "IAM to Billing endpoint"
  vpc_id             = var.infra.regions.frankfurt.vpc
  subnet_ids         = var.infra.regions.frankfurt.private_subnets
  security_group_ids = [
    var.iam_default_security_group_id,
  ]

  route53_record_zone_id = data.aws_route53_zone.billing_route53_zone.id
  route53_record_fqdn    = var.billing_endpoint_fqdn
}


# MDB
data "aws_route53_zone" "mdb_route53_zone" {
  name   = var.zone_mdb
  vpc_id = var.infra.regions.frankfurt.vpc
}

module "endpoint_mdb" {
  source = "../../../modules/general/endpoint/v1"

  service_name       = var.mdb_endpoint_service_name
  tag_name           = "IAM to MDB endpoint"
  vpc_id             = var.infra.regions.frankfurt.vpc
  subnet_ids         = var.infra.regions.frankfurt.private_subnets
  security_group_ids = [
    var.iam_default_security_group_id,
  ]

  route53_record_zone_id = data.aws_route53_zone.mdb_route53_zone.id
  route53_record_fqdn    = var.mdb_endpoint_fqdn
}


# VPC
data "aws_route53_zone" "vpc_route53_zone" {
  name   = var.zone_vpc
  vpc_id = var.infra.regions.frankfurt.vpc
}

module "endpoint_vpc" {
  source = "../../../modules/general/endpoint/v1"

  service_name       = var.vpc_endpoint_service_name
  tag_name           = "IAM to VPC endpoint"
  vpc_id             = var.infra.regions.frankfurt.vpc
  subnet_ids         = var.infra.regions.frankfurt.private_subnets
  security_group_ids = [
    var.iam_default_security_group_id,
  ]

  route53_record_zone_id = data.aws_route53_zone.vpc_route53_zone.id
  route53_record_fqdn    = var.vpc_endpoint_fqdn
}
