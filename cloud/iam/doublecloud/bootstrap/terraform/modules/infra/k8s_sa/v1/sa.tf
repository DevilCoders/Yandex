locals {
  role_name        = "k8s_cluster_${var.cluster_name}_aws_${var.sa_name}"
  role_policy_name = "k8s_cluster_${var.cluster_name}_aws_${var.sa_name}_policy"
}

data "aws_iam_policy_document" "sa_assume_role_policy" {
  statement {
    actions = ["sts:AssumeRoleWithWebIdentity"]
    effect  = "Allow"

    condition {
      test     = "StringEquals"
      variable = "${replace(var.openid_cluster_url, "https://", "")}:sub"
      values   = ["system:serviceaccount:${var.namespace}:${var.sa_name}"]
    }

    principals {
      identifiers = [var.openid_cluster_arn]
      type        = "Federated"
    }
  }
}

resource "aws_iam_role" "sa" {
  name               = local.role_name
  assume_role_policy = data.aws_iam_policy_document.sa_assume_role_policy.json
}

resource "aws_iam_role_policy" "sa_role" {
  name   = local.role_policy_name
  role   = aws_iam_role.sa.id
  policy = var.policy
}
