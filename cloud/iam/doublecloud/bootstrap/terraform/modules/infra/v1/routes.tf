resource "aws_route_table" "iam_frankfurt_a_private" {
    provider = aws.frankfurt
    vpc_id   = aws_vpc.vpc_iam_frankfurt.id

    route {
        cidr_block     = "0.0.0.0/0"
        nat_gateway_id = aws_nat_gateway.iam_frankfurt_a_nat.id
    }

    route {
        ipv6_cidr_block        = "::/0"
        egress_only_gateway_id = aws_egress_only_internet_gateway.iam_frankfurt_eigw.id
    }
}

resource "aws_route_table" "iam_frankfurt_b_private" {
    provider = aws.frankfurt
    vpc_id   = aws_vpc.vpc_iam_frankfurt.id

    route {
        cidr_block     = "0.0.0.0/0"
        nat_gateway_id = aws_nat_gateway.iam_frankfurt_b_nat.id
    }

    route {
        ipv6_cidr_block        = "::/0"
        egress_only_gateway_id = aws_egress_only_internet_gateway.iam_frankfurt_eigw.id
    }
}

resource "aws_route_table" "iam_frankfurt_c_private" {
    provider = aws.frankfurt
    vpc_id   = aws_vpc.vpc_iam_frankfurt.id

    route {
        cidr_block     = "0.0.0.0/0"
        nat_gateway_id = aws_nat_gateway.iam_frankfurt_c_nat.id
    }

    route {
        ipv6_cidr_block        = "::/0"
        egress_only_gateway_id = aws_egress_only_internet_gateway.iam_frankfurt_eigw.id
    }
}

resource "aws_route_table" "iam_frankfurt_public" {
    provider = aws.frankfurt
    vpc_id   = aws_vpc.vpc_iam_frankfurt.id

    route {
        cidr_block = "0.0.0.0/0"
        gateway_id = aws_internet_gateway.iam_frankfurt_igw.id
    }

    route {
        ipv6_cidr_block = "::/0"
        gateway_id      = aws_internet_gateway.iam_frankfurt_igw.id
    }

    dynamic "route" {
        for_each = toset(var.transit_gateways)
        content {
            transit_gateway_id = route.value.transit_gateway_id
            cidr_block         = route.value.cidr_block
        }
    }

    depends_on = [aws_ec2_transit_gateway_vpc_attachment.tgw_attachment]
}

resource "aws_route_table_association" "iam_frankfurt_a_private" {
    provider = aws.frankfurt

    route_table_id = aws_route_table.iam_frankfurt_a_private.id
    subnet_id      = aws_subnet.iam_frankfurt_a_private.id
}

resource "aws_route_table_association" "iam_frankfurt_a_public" {
    provider = aws.frankfurt

    route_table_id = aws_route_table.iam_frankfurt_public.id
    subnet_id      = aws_subnet.iam_frankfurt_a_public.id
}

resource "aws_route_table_association" "iam_frankfurt_b_private" {
    provider = aws.frankfurt

    route_table_id = aws_route_table.iam_frankfurt_b_private.id
    subnet_id      = aws_subnet.iam_frankfurt_b_private.id
}

resource "aws_route_table_association" "iam_frankfurt_b_public" {
    provider = aws.frankfurt

    route_table_id = aws_route_table.iam_frankfurt_public.id
    subnet_id      = aws_subnet.iam_frankfurt_b_public.id
}

resource "aws_route_table_association" "iam_frankfurt_c_private" {
    provider = aws.frankfurt

    route_table_id = aws_route_table.iam_frankfurt_c_private.id
    subnet_id      = aws_subnet.iam_frankfurt_c_private.id
}

resource "aws_route_table_association" "iam_frankfurt_c_public" {
    provider = aws.frankfurt

    route_table_id = aws_route_table.iam_frankfurt_public.id
    subnet_id      = aws_subnet.iam_frankfurt_c_public.id
}