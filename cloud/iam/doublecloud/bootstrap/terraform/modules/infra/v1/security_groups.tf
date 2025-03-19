module "security_groups" {
    source          = "../../../modules/infra/sg/v1"

    infra_name      = var.infra_name
    vpc_id          = aws_vpc.vpc_iam_frankfurt.id

    cidr_block      = aws_vpc.vpc_iam_frankfurt.cidr_block
    ipv6_cidr_block = aws_vpc.vpc_iam_frankfurt.ipv6_cidr_block
}
