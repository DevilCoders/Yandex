locals {
  vector_namespace   = "logging"
  elasticsearch_host = tolist(data.yandex_mdb_elasticsearch_cluster.vector_elastic.host)[0].fqdn
}

provider "elasticstack" {
  elasticsearch {
    username  = "admin"
    password  = random_password.vector_elastic_admin_password.result
    endpoints = ["https://${local.elasticsearch_host}:9200"]
  }
}

data "yandex_mdb_elasticsearch_cluster" "vector_elastic" {
  name       = "vector-elastic"
  depends_on = [yandex_mdb_elasticsearch_cluster.vector_elastic]
}

resource "yandex_vpc_security_group" "cloudbeaver-kibana-sg" {
  name        = "cloudbeaver-kibana"
  description = "CloudBeaver elasticsearch kibana access"
  network_id  = data.ycp_vpc_network.cloudbeaver.id
  ingress {
    protocol       = "TCP"
    description    = "Allow incominng traffic to 443 (HTTPS) from yandexnets"
    port           = 443
    v4_cidr_blocks = local.yandexnets.ipv4
    v6_cidr_blocks = local.yandexnets.ipv6
  }
  ingress {
    protocol          = "TCP"
    port              = 9200
    security_group_id = module.k8s.node_security_group_id
  }
}

resource "kubernetes_namespace" "vector_namespace" {
  provider = kubernetes.kubernetes
  metadata {
    name = local.vector_namespace
  }
}

resource "random_password" "vector_elastic_admin_password" {
  length  = 12
  special = true
}


resource "yandex_mdb_elasticsearch_cluster" "vector_elastic" {
  name               = "vector-elastic"
  folder_id          = var.folder_id
  environment        = "PRODUCTION"
  network_id         = var.network_id
  security_group_ids = [yandex_vpc_security_group.cloudbeaver-kibana-sg.id]

  labels = {
    mdb-auto-purge = "off"
  }

  config {

    admin_password = random_password.vector_elastic_admin_password.result

    data_node {
      resources {
        resource_preset_id = "s2.micro"
        disk_type_id       = "network-ssd"
        disk_size          = 100
      }
    }

  }

  host {
    name             = "node"
    zone             = "ru-central1-a"
    type             = "DATA_NODE"
    assign_public_ip = true
    subnet_id        = data.ycp_vpc_subnet.ru-central1-a.subnet_id
  }
}

#resource "elasticstack_elasticsearch_security_user" "dev" {
#  username = "devuser"
#  password = "1234567890"
#  roles    = ["kibana_user"]
#}
