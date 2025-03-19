resource "aws_vpc_endpoint_service" "iam_nlb_tls_endpoint_service" {
  acceptance_required        = var.endpoint_service_acceptance_required
  allowed_principals         = var.endpoint_service_allowed_principals

  network_load_balancer_arns = [
    module.iam_tls_nlb.lb_arn,
  ]

  tags = {
    Name = "${var.k8s_cluster_name} tls nlb"
  }
}
