output "endpoint" {
  value = yandex_kubernetes_cluster.k8s.master[0].external_v4_endpoint
}

output "ca" {
  value     = yandex_kubernetes_cluster.k8s.master[0].cluster_ca_certificate
  sensitive = true
}

output "cluster_security_group" {
  value = yandex_vpc_security_group.k8s-main-sg.id
}

output "cluster_id" {
  value = yandex_kubernetes_cluster.k8s.id
}

output "node_security_group_id" {
  value = yandex_vpc_security_group.k8s-public-services.id
}
