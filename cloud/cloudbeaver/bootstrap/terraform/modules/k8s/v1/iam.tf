resource "yandex_container_registry_iam_binding" "puller" {
  registry_id = var.registry_id
  role        = "container-registry.images.puller"

  members = [
    "serviceAccount:${yandex_iam_service_account.k8s_node.id}",
  ]
}


resource "yandex_container_registry_iam_binding" "pusher" {
  registry_id = var.registry_id
  role        = "container-registry.images.pusher"

  members = [
    "serviceAccount:${var.docker_registry_pusher}",
  ]
}


resource "yandex_iam_service_account" "k8s_cluster" {
  name = "k8s-cluster-${var.cluster_name}"
}

resource "yandex_iam_service_account" "k8s_node" {
  name = "k8s-node-${var.cluster_name}"
}

resource "yandex_resourcemanager_folder_iam_member" "k8s_cluster_editor" {
  folder_id = var.folder_id
  member    = "serviceAccount:${yandex_iam_service_account.k8s_cluster.id}"
  role      = "editor"
}

#resource "yandex_resourcemanager_folder_iam_member" "k8s_cluster_role" {
#  folder_id = var.folder_id
#  member    = "serviceAccount:${yandex_iam_service_account.k8s_cluster.id}"
#  role      = "mdb.k8sCluster"
#}
