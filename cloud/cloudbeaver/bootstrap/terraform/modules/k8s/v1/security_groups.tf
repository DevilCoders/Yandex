resource "yandex_vpc_security_group" "k8s-main-sg" {
  network_id  = var.network_id
  name        = "k8s-main-${var.cluster_name}"
  description = "as in https://cloud.yandex.ru/docs/managed-kubernetes/operations/security-groups#examples"
  ingress {
    protocol       = "TCP"
    description    = "Load balancing"
    v4_cidr_blocks = var.healthchecks_cidrs.v4
    v6_cidr_blocks = var.healthchecks_cidrs.v6
    from_port      = 0
    to_port        = 65535
  }
  ingress {
    protocol          = "ANY"
    description       = "Any to any in kuberenets sg"
    predefined_target = "self_security_group"
    from_port         = 0
    to_port           = 65535
  }
  ingress {
    protocol    = "ANY"
    description = "pod-to-pod, service-to-service"
    v4_cidr_blocks = [
      var.service_ipv4_range,
      var.cluster_ipv4_range
    ]
    v6_cidr_blocks = [
      var.service_ipv6_range,
      var.cluster_ipv6_range
    ]
    from_port = 0
    to_port   = 65535
  }
  ingress {
    protocol       = "ICMP"
    description    = "ICMP"
    v4_cidr_blocks = var.healthchecks_cidrs.v4
    v6_cidr_blocks = var.healthchecks_cidrs.v6
    from_port      = 0
    to_port        = 32767
  }
  egress {
    protocol       = "ANY"
    description    = "All outgoing traffic. For Yandex Object Storage, Yandex Container Registry, Docker Hub etc"
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
    from_port      = 0
    to_port        = 65535
  }
}

resource "yandex_vpc_security_group" "k8s-public-services" {
  name        = "k8s-public-services-${var.cluster_name}"
  description = "Only for nodes. As in https://cloud.yandex.ru/docs/managed-kubernetes/operations/security-groups#examples"
  network_id  = var.network_id

  ingress {
    protocol       = "TCP"
    description    = "Allow incoming trafic from healthchecks nets"
    v4_cidr_blocks = var.healthchecks_cidrs.v4
    v6_cidr_blocks = var.healthchecks_cidrs.v6
    from_port      = 0
    to_port        = 65535
  }
  ingress {
    protocol       = "TCP"
    description    = "Allow incoming traffic from yandexnets"
    v4_cidr_blocks = var.yandex_nets.ipv4
    v6_cidr_blocks = var.yandex_nets.ipv6
    from_port      = 0
    to_port        = 65535
  }
}

resource "yandex_vpc_security_group" "k8s-nodes-ssh-access" {
  name        = "k8s-nodes-ssh-access-${var.cluster_name}"
  description = "Only for nodes. SSH connection. As in https://cloud.yandex.ru/docs/managed-kubernetes/operations/security-groups#examples"
  network_id  = var.network_id

  ingress {
    protocol       = "TCP"
    description    = "SSH"
    v4_cidr_blocks = var.yandex_nets.ipv4
    v6_cidr_blocks = var.yandex_nets.ipv6
    port           = 22
  }
}

resource "yandex_vpc_security_group" "k8s-master-whitelist" {
  name        = "k8s-master-whitelist-${var.cluster_name}"
  description = "Only for cluster. Access Kubernetes API from internet. As in https://cloud.yandex.ru/docs/managed-kubernetes/operations/security-groups#examples."
  network_id  = var.network_id

  ingress {
    protocol       = "TCP"
    description    = "Access Kubernetes API on 6443"
    v4_cidr_blocks = var.yandex_nets.ipv4
    v6_cidr_blocks = var.yandex_nets.ipv6
    port           = 6443
  }

  ingress {
    protocol       = "TCP"
    description    = "Access Kubernetes API on 443"
    v4_cidr_blocks = var.yandex_nets.ipv4
    v6_cidr_blocks = var.yandex_nets.ipv6
    port           = 443
  }
}
