resource "helm_release" "certmanager" {
  name             = "cert-manager"
  repository       = "https://charts.jetstack.io"
  chart            = "cert-manager"
  version          = "v1.3.1"
  namespace        = "cert-manager"
  create_namespace = true
  cleanup_on_fail  = true
  timeout          = 300
  wait_for_jobs    = true
  atomic           = true

  // TODO: this option is dangerous, think about safer solution
  // https://artifacthub.io/packages/helm/cert-manager/cert-manager
  // If true, CRD resources will be installed as part of the Helm chart. If enabled, when uninstalling CRD resources will be deleted causing all installed custom resources to be DELETED
  set {
    name  = "installCRDs"
    value = "true"
  }

  depends_on = [
    aws_eks_addon.cni
  ]
}
