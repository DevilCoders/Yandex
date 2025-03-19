resource "aws_kms_key" "auth_ui_helmsecrets_key" {
  description = "auth-ui ${var.infra_name} KMS-key for helm-secrets"
}

resource "aws_iam_policy" "auth_ui_KMS_HelmSecretsKey" {
  name = "auth_ui_${var.infra_name}_KMS_HelmSecretsKey"

  policy = <<POLICY
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "HelmSecretsKey",
            "Effect": "Allow",
            "Action": [
                "kms:Encrypt",
                "kms:Decrypt",
                "kms:ReEncrypt*",
                "kms:GenerateDataKey*",
                "kms:DescribeKey"
            ],
            "Resource": "${aws_kms_key.auth_ui_helmsecrets_key.arn}"
        }
    ]
}
POLICY
}

resource "aws_iam_policy_attachment" "auth_ui_KMS_HelmSecretsKey" {
  name       = "auth_ui_${var.infra_name}_KMS_HelmSecretsKey"
  users      = var.auth_ui_devops
  policy_arn = aws_iam_policy.auth_ui_KMS_HelmSecretsKey.arn
}
