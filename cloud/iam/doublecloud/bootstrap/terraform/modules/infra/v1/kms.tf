resource "aws_kms_key" "iam_k8s_key" {
  description = "IAM ${var.infra_name} KMS-key of k8s secure cluster Secrets"
}

resource "aws_kms_key" "iam_helmsecrets_key" {
  description = "IAM ${var.infra_name} KMS-key for helm-secrets (datacloud-aws)"
}
