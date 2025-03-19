data "aws_iam_policy_document" "prometheus_reader_assume_role" {
  statement {
    actions = ["sts:AssumeRole"]
    principals {
      identifiers = sort([for acc in var.prometheus_reader_accounts : "arn:aws:iam::${acc}:root"])
      type        = "AWS"
    }
  }
}

data "aws_iam_policy_document" "prometheus_reader_policy" {
  statement {
    actions = [
      "aps:DescribeAlertManagerDefinition",
      "aps:DescribeRuleGroupsNamespace",
      "aps:DescribeWorkspace",
      "aps:GetAlertManagerSilence",
      "aps:GetAlertManagerStatus",
      "aps:GetLabels",
      "aps:GetMetricMetadata",
      "aps:GetSeries",
      "aps:ListAlertManagerAlertGroups",
      "aps:ListAlertManagerAlerts",
      "aps:ListAlertManagerReceivers",
      "aps:ListAlertManagerSilences",
      "aps:ListAlerts",
      "aps:ListRules",
      "aps:ListRuleGroupsNamespaces",
      "aps:ListTagsForResource",
      "aps:ListWorkspaces",
      "aps:QueryMetrics"
    ]
    effect    = "Allow"
    resources = ["*"]
  }
}

resource "aws_iam_role" "prometheus_reader" {
  name               = "prometheus-reader"
  assume_role_policy = data.aws_iam_policy_document.prometheus_reader_assume_role.json
}

resource "aws_iam_role_policy" "prometheus_reader" {
  name   = "prometheus-reader"
  role   = aws_iam_role.prometheus_reader.id
  policy = data.aws_iam_policy_document.prometheus_reader_policy.json
}
