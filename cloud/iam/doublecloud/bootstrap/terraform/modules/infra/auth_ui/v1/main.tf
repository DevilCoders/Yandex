resource "aws_iam_policy" "auth_ui_ECR_ManageRepositoryContents" {
  name = "${var.infra_name}_ECR_ManageRepositoryContents"

  policy = <<POLICY
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "ListImagesInRepository",
            "Effect": "Allow",
            "Action": [
                "ecr:ListImages"
            ],
            "Resource": "${var.auth_ui_repository_arn}"
        },
        {
            "Sid": "GetAuthorizationToken",
            "Effect": "Allow",
            "Action": [
                "ecr:GetAuthorizationToken"
            ],
            "Resource": "*"
        },
        {
            "Sid": "ManageRepositoryContents",
            "Effect": "Allow",
            "Action": [
                "ecr:BatchCheckLayerAvailability",
                "ecr:GetDownloadUrlForLayer",
                "ecr:GetRepositoryPolicy",
                "ecr:DescribeRepositories",
                "ecr:ListImages",
                "ecr:DescribeImages",
                "ecr:BatchGetImage",
                "ecr:InitiateLayerUpload",
                "ecr:UploadLayerPart",
                "ecr:CompleteLayerUpload",
                "ecr:PutImage"
            ],
            "Resource": "${var.auth_ui_repository_arn}"
        }
    ]
}
POLICY
}

resource "aws_iam_policy_attachment" "auth_ui_ECR_ManageRepositoryContents" {
  name       = "${var.infra_name}_auth_ui_ECR_ManageRepositoryContents"
  users      = var.auth_ui_devops
  policy_arn = aws_iam_policy.auth_ui_ECR_ManageRepositoryContents.arn
}

resource "aws_iam_policy" "ManageOwnAccessKeys" {
  name = "${var.infra_name}_ManageOwnAccessKeys"

  policy = <<POLICY
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "ManageOwnAccessKeys",
            "Effect": "Allow",
            "Action": [
                "iam:CreateAccessKey",
                "iam:DeleteAccessKey",
                "iam:GetAccessKeyLastUsed",
                "iam:GetUser",
                "iam:ListAccessKeys",
                "iam:UpdateAccessKey"
            ],
            "Resource": "arn:aws:iam::*:user/$${aws:username}"
        }
    ]
}
POLICY
}

resource "aws_iam_policy_attachment" "auth_ui_ManageOwnAccessKeys" {
  name       = "${var.infra_name}_auth_ui_ManageOwnAccessKeys"
  users      = var.auth_ui_devops
  policy_arn = aws_iam_policy.ManageOwnAccessKeys.arn
}
