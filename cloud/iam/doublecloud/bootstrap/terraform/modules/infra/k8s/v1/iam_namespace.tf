resource "kubernetes_namespace" "iam" {
  metadata {
    name = "iam"

    labels = {
      "elbv2.k8s.aws/pod-readiness-gate-inject" = "enabled"
    }
  }

  depends_on = [
    aws_eks_cluster.cluster,
  ]
}
