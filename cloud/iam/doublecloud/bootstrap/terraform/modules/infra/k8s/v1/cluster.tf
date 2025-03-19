resource "aws_iam_role" "cluster" {
  name = "k8s_cluster_${var.cluster_name}"

  assume_role_policy = <<POLICY
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Principal": {
        "Service": "eks.amazonaws.com"
      },
      "Action": "sts:AssumeRole"
    }
  ]
}
POLICY
}

resource "aws_iam_role_policy_attachment" "cluster-AmazonEKSClusterPolicy" {
  policy_arn = "arn:aws:iam::aws:policy/AmazonEKSClusterPolicy"
  role       = aws_iam_role.cluster.name
}

resource "aws_eks_cluster" "cluster" {
  name     = var.cluster_name
  role_arn = aws_iam_role.cluster.arn
  version  = "1.19"

  vpc_config {
    subnet_ids              = var.cluster_subnet_ids
    endpoint_private_access = true
    endpoint_public_access  = true
  }

  dynamic "encryption_config" {
    for_each = toset(var.cluster_encryption_config)

    content {
      provider {
        key_arn = encryption_config.value["provider_key_arn"]
      }
      resources = encryption_config.value["resources"]
    }
  }

  depends_on = [
    aws_iam_role_policy_attachment.cluster-AmazonEKSClusterPolicy
  ]
}

// Allow ingress from within our vpc (so that LB can access services within k8s, etc)
resource "aws_security_group_rule" "allow_within_vpc" {
  type              = "ingress"
  security_group_id = aws_eks_cluster.cluster.vpc_config[0].cluster_security_group_id
  cidr_blocks       = var.ingress_cidrs
  ipv6_cidr_blocks  = var.ingress_ipv6_cidrs
  protocol          = "-1"
  from_port         = 0
  to_port           = 0
}

resource "aws_eks_addon" "cni" {
  cluster_name      = aws_eks_cluster.cluster.name
  addon_name        = "vpc-cni"
  addon_version     = "v1.7.10-eksbuild.1"
  resolve_conflicts = "OVERWRITE"

  depends_on = [
    // Without node groups dependency resource creating will hang.
    module.node_groups
  ]
}

resource "aws_iam_policy" "eks_cluster" {
  name = "${var.cluster_name}_policy"

  policy = <<POLICY
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": [
                "eks:DescribeCluster",
                "eks:ListClusters"
            ],
            "Resource": "${aws_eks_cluster.cluster.arn}"
        }
    ]
}
POLICY
}

resource "aws_iam_policy_attachment" "eks_cluster" {
  name       = "${var.cluster_name}_policy_attachment"
  users      = concat(var.iam_devops, var.auth_ui_devops)
  policy_arn = aws_iam_policy.eks_cluster.arn
}
