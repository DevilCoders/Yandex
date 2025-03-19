resource "aws_ecr_repository" "iam-access-service-application" {
    name                 = "iam-access-service-application"
    image_tag_mutability = "MUTABLE"

    image_scanning_configuration {
        scan_on_push = true
    }
}

resource "aws_ecr_repository" "iam-control-plane-application" {
    name                 = "iam-control-plane-application"
    image_tag_mutability = "MUTABLE"

    image_scanning_configuration {
        scan_on_push = true
    }
}

resource "aws_ecr_repository" "iam-datacloud-identity" {
    name                 = "iam-datacloud-identity"
    image_tag_mutability = "MUTABLE"

    image_scanning_configuration {
        scan_on_push = true
    }
}

resource "aws_ecr_repository" "iam-mfa-service-application" {
    name                 = "iam-mfa-service-application"
    image_tag_mutability = "MUTABLE"

    image_scanning_configuration {
        scan_on_push = true
    }
}

resource "aws_ecr_repository" "iam-openid-server" {
    name                 = "iam-openid-server"
    image_tag_mutability = "MUTABLE"

    image_scanning_configuration {
        scan_on_push = true
    }
}

resource "aws_ecr_repository" "iam-org-service" {
    name                 = "iam-org-service"
    image_tag_mutability = "MUTABLE"

    image_scanning_configuration {
        scan_on_push = true
    }
}

resource "aws_ecr_repository" "resource-manager-control-plane-application" {
    name                 = "resource-manager-control-plane-application"
    image_tag_mutability = "MUTABLE"

    image_scanning_configuration {
        scan_on_push = true
    }
}

resource "aws_ecr_repository" "iam-token-service-application" {
    name                 = "iam-token-service-application"
    image_tag_mutability = "MUTABLE"

    image_scanning_configuration {
        scan_on_push = true
    }
}

resource "aws_ecr_repository" "iam-sync" {
    name                 = "iam-sync"
    image_tag_mutability = "MUTABLE"

    image_scanning_configuration {
        scan_on_push = true
    }
}

resource "aws_ecr_repository" "auth-ui" {
    name                 = "auth-ui"
    image_tag_mutability = "MUTABLE"

    image_scanning_configuration {
        scan_on_push = true
    }
}
