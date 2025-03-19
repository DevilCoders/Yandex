output "docker_registry_id" {
  value = yandex_container_registry.cloudbeaver.id
}

output "kubernetes_cluster_id" {
  value = module.k8s.cluster_id
}

output "s3fs_static_access_key" {
  value = yandex_iam_service_account_static_access_key.s3fs_access_key.access_key
}

output "s3fs_static_access_key_secret" {
  value     = yandex_iam_service_account_static_access_key.s3fs_access_key.secret_key
  sensitive = true
}

output "elastic_admin_password" {
  value     = random_password.vector_elastic_admin_password.result
  sensitive = true
}
