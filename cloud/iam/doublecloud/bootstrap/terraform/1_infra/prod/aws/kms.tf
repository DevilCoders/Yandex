resource "aws_kms_key" "iam_prod_infraplane_helmsecrets_key" {
  description = "IAM prod KMS-key for infraplane helm-secrets"
}
