resource "aws_internet_gateway" "iam_frankfurt_igw" {
    provider = aws.frankfurt
    vpc_id   = aws_vpc.vpc_iam_frankfurt.id

    tags = {
        Name = "iam"
    }
}

resource "aws_egress_only_internet_gateway" "iam_frankfurt_eigw" {
    provider = aws.frankfurt
    vpc_id   = aws_vpc.vpc_iam_frankfurt.id

    tags = {
        Name = "iam"
    }
}

resource "aws_eip" "iam_frankfurt_a_eip" {
    provider = aws.frankfurt
    vpc      = true

    tags = {
        Name = "iam nat zone-a"
    }
}

resource "aws_eip" "iam_frankfurt_b_eip" {
    provider = aws.frankfurt
    vpc      = true

    tags = {
        Name = "iam nat zone-b"
    }
}

resource "aws_eip" "iam_frankfurt_c_eip" {
    provider = aws.frankfurt
    vpc      = true

    tags = {
        Name = "iam nat zone-c"
    }
}

resource "aws_nat_gateway" "iam_frankfurt_a_nat" {
    provider      = aws.frankfurt
    allocation_id = aws_eip.iam_frankfurt_a_eip.id
    subnet_id     = aws_subnet.iam_frankfurt_a_public.id

    tags = {
        Name = "iam zone-a"
    }
}

resource "aws_nat_gateway" "iam_frankfurt_b_nat" {
    provider      = aws.frankfurt
    allocation_id = aws_eip.iam_frankfurt_b_eip.id
    subnet_id     = aws_subnet.iam_frankfurt_b_public.id

    tags = {
        Name = "iam zone-b"
    }
}

resource "aws_nat_gateway" "iam_frankfurt_c_nat" {
    provider      = aws.frankfurt
    allocation_id = aws_eip.iam_frankfurt_c_eip.id
    subnet_id     = aws_subnet.iam_frankfurt_c_public.id

    tags = {
        Name = "iam zone-c"
    }
}
