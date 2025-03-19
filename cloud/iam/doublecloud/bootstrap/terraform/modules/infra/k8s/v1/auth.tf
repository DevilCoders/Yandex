locals {
  configmap_roles = [
    {
      rolearn  = "arn:aws:iam::${var.aws_account}:role/${aws_iam_role.node_group.name}"
      username = "system:node:{{EC2PrivateDNSName}}"
      groups = [
        "system:bootstrappers",
        "system:nodes",
      ]
    }
  ]

  kubeconfig_users = [for user in concat(var.iam_devops, var.auth_ui_devops) : {
    userarn  = "arn:aws:iam::${var.aws_account}:user/${user}"
    username = user
    groups   = ["system:masters"]
  }]
}

resource "kubernetes_config_map" "k8s_auth" {
  metadata {
    name      = "aws-auth"
    namespace = "kube-system"
  }

  data = {
    mapRoles = yamlencode(local.configmap_roles)
    mapUsers = yamlencode(local.kubeconfig_users)
  }

  depends_on = [aws_eks_cluster.cluster]
}
