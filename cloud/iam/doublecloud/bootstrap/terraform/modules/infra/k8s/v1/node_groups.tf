resource "aws_iam_role" "node_group" {
  name = "k8s_cluster_${var.cluster_name}_node_group"

  assume_role_policy = <<POLICY
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Principal": {
        "Service": "ec2.amazonaws.com"
      },
      "Action": "sts:AssumeRole"
    }
  ]
}
POLICY
}

module "node_groups" {
  source = "./node_group"

  for_each = {
    for node_group in var.node_groups : node_group.name => node_group
  }

  cluster_name       = aws_eks_cluster.cluster.name
  ami_version        = var.ami_version
  node_group_name    = each.value.name
  node_group_role    = aws_iam_role.node_group
  subnet_ids         = each.value.subnet_ids
  security_group_ids = each.value.security_group_ids
  labels             = each.value.labels
  instance_types     = each.value.instance_types
  scaling_config     = each.value.scaling_config
  ssh_key_name       = var.ssh_key_name
}

