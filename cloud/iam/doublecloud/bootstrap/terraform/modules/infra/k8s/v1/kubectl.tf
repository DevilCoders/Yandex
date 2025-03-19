resource "local_file" "kubeconfig" {
  content              = local.kubeconfig
  filename             = "./.kubeconfig_${var.cluster_name}"
  file_permission      = "0600"
  directory_permission = "0755"
}
