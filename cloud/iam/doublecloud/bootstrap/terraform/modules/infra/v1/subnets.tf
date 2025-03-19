resource "aws_subnet" "iam_frankfurt_a_private" {
    provider             = aws.frankfurt
    cidr_block           = cidrsubnet(aws_vpc.vpc_iam_frankfurt.cidr_block, 2, 0)
    vpc_id               = aws_vpc.vpc_iam_frankfurt.id
    ipv6_cidr_block      = cidrsubnet(aws_vpc.vpc_iam_frankfurt.ipv6_cidr_block, 8, 0)
    availability_zone_id = module.aws.regions[var.region].zones.a.id

    assign_ipv6_address_on_creation = true

    tags = {
        Name = "iam-frankfurt-a-private"
    }
}

resource "aws_subnet" "iam_frankfurt_a_public" {
    provider             = aws.frankfurt
    cidr_block           = cidrsubnet(cidrsubnet(aws_vpc.vpc_iam_frankfurt.cidr_block, 2, 3), 2, 0)
    vpc_id               = aws_vpc.vpc_iam_frankfurt.id
    ipv6_cidr_block      = cidrsubnet(aws_vpc.vpc_iam_frankfurt.ipv6_cidr_block, 8, 3)
    availability_zone_id = module.aws.regions[var.region].zones.a.id

    assign_ipv6_address_on_creation = true
    map_public_ip_on_launch         = true

    tags = {
        Name = "iam-frankfurt-a-public"
    }
}

resource "aws_subnet" "iam_frankfurt_b_private" {
    provider             = aws.frankfurt
    cidr_block           = cidrsubnet(aws_vpc.vpc_iam_frankfurt.cidr_block, 2, 1)
    vpc_id               = aws_vpc.vpc_iam_frankfurt.id
    ipv6_cidr_block      = cidrsubnet(aws_vpc.vpc_iam_frankfurt.ipv6_cidr_block, 8, 1)
    availability_zone_id = module.aws.regions[var.region].zones.b.id

    assign_ipv6_address_on_creation = true

    tags = {
        Name = "iam-frankfurt-b-private"
    }
}

resource "aws_subnet" "iam_frankfurt_b_public" {
    provider             = aws.frankfurt
    cidr_block           = cidrsubnet(cidrsubnet(aws_vpc.vpc_iam_frankfurt.cidr_block, 2, 3), 2, 1)
    vpc_id               = aws_vpc.vpc_iam_frankfurt.id
    ipv6_cidr_block      = cidrsubnet(aws_vpc.vpc_iam_frankfurt.ipv6_cidr_block, 8, 4)
    availability_zone_id = module.aws.regions[var.region].zones.b.id

    assign_ipv6_address_on_creation = true
    map_public_ip_on_launch         = true

    tags = {
        Name = "iam-frankfurt-b-public"
    }
}

resource "aws_subnet" "iam_frankfurt_c_private" {
    provider             = aws.frankfurt
    cidr_block           = cidrsubnet(aws_vpc.vpc_iam_frankfurt.cidr_block, 2, 2)
    vpc_id               = aws_vpc.vpc_iam_frankfurt.id
    ipv6_cidr_block      = cidrsubnet(aws_vpc.vpc_iam_frankfurt.ipv6_cidr_block, 8, 2)
    availability_zone_id = module.aws.regions[var.region].zones.c.id

    assign_ipv6_address_on_creation = true

    tags = {
        Name = "iam-frankfurt-c-private"
    }
}

resource "aws_subnet" "iam_frankfurt_c_public" {
    provider             = aws.frankfurt
    cidr_block           = cidrsubnet(cidrsubnet(aws_vpc.vpc_iam_frankfurt.cidr_block, 2, 3), 2, 2)
    vpc_id               = aws_vpc.vpc_iam_frankfurt.id
    ipv6_cidr_block      = cidrsubnet(aws_vpc.vpc_iam_frankfurt.ipv6_cidr_block, 8, 5)
    availability_zone_id = module.aws.regions[var.region].zones.c.id

    assign_ipv6_address_on_creation = true
    map_public_ip_on_launch         = true

    tags = {
        Name = "iam-frankfurt-c-public"
    }
}

locals {
    private_subnets_ids = [
        aws_subnet.iam_frankfurt_a_private.id,
        aws_subnet.iam_frankfurt_b_private.id,
        aws_subnet.iam_frankfurt_c_private.id,
    ]
    public_subnets_ids  = [
        aws_subnet.iam_frankfurt_a_public.id,
        aws_subnet.iam_frankfurt_b_public.id,
        aws_subnet.iam_frankfurt_c_public.id,
    ]
}
