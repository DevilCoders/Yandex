resource "aws_vpc" "vpc_iam_frankfurt" {
    cidr_block                       = module.aws.regions[var.region].vpc_cidr
    provider                         = aws.frankfurt
    assign_generated_ipv6_cidr_block = true

    enable_dns_hostnames = true
    enable_dns_support   = true

    tags = {
        Name = "iam-frankfurt"
    }
}
