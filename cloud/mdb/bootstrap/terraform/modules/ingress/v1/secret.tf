resource "kubernetes_secret" "yc_alb_ingress_controller_sa_key" {
  metadata {
    name      = "${local.chart_name}-sa-key"
    namespace = kubernetes_namespace.namespace.metadata[0].name
  }
  data = {
    "sa-key.json" = jsonencode({
      id                 = yandex_iam_service_account_key.ingress_key.id
      created_at         = yandex_iam_service_account_key.ingress_key.created_at
      key_algorithm      = yandex_iam_service_account_key.ingress_key.key_algorithm
      public_key         = yandex_iam_service_account_key.ingress_key.public_key
      private_key        = yandex_iam_service_account_key.ingress_key.private_key
      service_account_id = yandex_iam_service_account_key.ingress_key.service_account_id
    })
  }
}
