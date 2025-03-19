resource "aws_kms_key" "iam_preprod_yc_helmsecrets_key" {
  description = "IAM preprod KMS-key for helm-secrets (datacloud-yc)"
}

resource "aws_kms_key" "auth_ui_helmsecrets_key" {
  description = "auth-ui preprod KMS-key for helm-secrets"
}
