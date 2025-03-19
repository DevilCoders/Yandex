resource "aws_iam_role_policy_attachment" "node-AmazonEKSWorkerNodePolicy" {
  policy_arn = "arn:aws:iam::aws:policy/AmazonEKSWorkerNodePolicy"
  role       = var.node_group_role.name
}

resource "aws_iam_role_policy_attachment" "node-AmazonEKS_CNI_Policy" {
  policy_arn = "arn:aws:iam::aws:policy/AmazonEKS_CNI_Policy"
  role       = var.node_group_role.name
}

resource "aws_iam_role_policy_attachment" "node-AmazonEC2ContainerRegistryReadOnly" {
  policy_arn = "arn:aws:iam::aws:policy/AmazonEC2ContainerRegistryReadOnly"
  role       = var.node_group_role.name
}

resource "aws_iam_role_policy" "node-SecretsManagerGetSecretValue" {
  name = "k8s_cluster_${var.cluster_name}_node_${var.node_group_name}_SecretsManagerGetSecretValue"
  role = var.node_group_role.id

  policy = jsonencode({
    "Version" : "2012-10-17",
    "Statement" : [
      {
        "Sid" : "VisualEditor0",
        "Effect" : "Allow",
        "Action" : "secretsmanager:GetSecretValue",
        "Resource" : "*"
      }
    ]
  })
}

resource "aws_eks_node_group" "node_group" {
  cluster_name    = var.cluster_name
  release_version = var.ami_version
  node_group_name = var.node_group_name
  node_role_arn   = var.node_group_role.arn
  subnet_ids      = var.subnet_ids
  labels          = var.labels

  scaling_config {
    desired_size = var.scaling_config.desired_size
    max_size     = var.scaling_config.max_size
    min_size     = var.scaling_config.min_size
  }

  instance_types = var.instance_types

  remote_access {
    ec2_ssh_key               = var.ssh_key_name
    source_security_group_ids = var.security_group_ids
  }

  tags = {
    Name = "${var.cluster_name} ${var.node_group_name} node-group"
  }

  depends_on = [
    aws_iam_role_policy_attachment.node-AmazonEKSWorkerNodePolicy,
    aws_iam_role_policy_attachment.node-AmazonEKS_CNI_Policy,
    aws_iam_role_policy_attachment.node-AmazonEC2ContainerRegistryReadOnly,
    aws_iam_role_policy.node-SecretsManagerGetSecretValue,
  ]
}
