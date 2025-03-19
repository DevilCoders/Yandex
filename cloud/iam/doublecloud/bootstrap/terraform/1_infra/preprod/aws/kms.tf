resource "aws_kms_key" "iam_preprod_yc_helmsecrets_key" {
  description = "IAM preprod KMS-key for helm-secrets (datacloud-yc)"
}
