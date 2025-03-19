terraform {
  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
  }
  required_version = ">= 0.13"
}

module "common" {
  source = "../common"
}

provider "ycp" {
  prod        = false
  ycp_profile = module.common.ycp_profile
  folder_id   = var.yc_folder
  zone        = module.common.yc_zone
}

locals {
  v6-yandex           = ["2a02:6b8::/32"]
  v6-cloud            = ["2a0d:d6c0::/29"]
  v6-yandex-and-cloud = ["2a02:6b8::/32", "2a0d:d6c0::/29"]
  v4-any              = ["0.0.0.0/0"]
  v6-any              = ["::/0"]
  v4-l4lb-healthcheck = ["198.18.235.0/24", "198.18.248.0/24"]
}

resource ycp_vpc_security_group common-security-group {
  name       = "default"
  network_id = var.network_id

  rule_spec {
    cidr_blocks {
      v6_cidr_blocks = local.v6-yandex-and-cloud
    }
    description   = "SSH"
    direction     = "INGRESS"
    ports {
      from_port = 22
      to_port   = 22
    }
    protocol_name = "TCP"
  }

  rule_spec {
    cidr_blocks {
      v6_cidr_blocks = local.v6-yandex-and-cloud
    }
    description   = "HTTPS/GRPC"
    direction     = "INGRESS"
    ports {
      from_port = 443
      to_port   = 443
    }
    protocol_name = "TCP"
  }

  rule_spec {
    cidr_blocks {
      v6_cidr_blocks = local.v6-yandex-and-cloud
    }
    description   = "Solomon"
    direction     = "INGRESS"
    ports {
      from_port = 8080
      to_port   = 8080
    }
    protocol_name = "TCP"
  }

  rule_spec {
    cidr_blocks {
      v6_cidr_blocks = local.v6-yandex-and-cloud
    }
    description   = "Ingress ICMP"
    direction     = "INGRESS"
    ports {
      from_port = 0
      to_port   = 32767
    }
    protocol_name = "IPV6_ICMP"
  }

  rule_spec {
    cidr_blocks {
      v6_cidr_blocks = local.v6-yandex-and-cloud
    }
    description   = "Egress TCP"
    direction     = "EGRESS"
    ports {
      from_port = 0
      to_port   = 32767
    }
    protocol_name = "TCP"
  }

  rule_spec {
    cidr_blocks {
      v6_cidr_blocks = local.v6-yandex-and-cloud
    }
    description   = "Egress UDP"
    direction     = "EGRESS"
    ports {
      from_port = 0
      to_port   = 32767
    }
    protocol_name = "UDP"
  }

  rule_spec {
    cidr_blocks {
      v6_cidr_blocks = local.v6-yandex-and-cloud
    }
    description   = "Egress ICMP"
    direction     = "EGRESS"
    ports {
      from_port = 0
      to_port   = 32767
    }
    protocol_name = "IPV6_ICMP"
  }

  rule_spec {
    description       = "Self"
    direction         = "INGRESS"
    ports {
      from_port = 0
      to_port   = 65535
    }
    predefined_target = "self_security_group"
    protocol_name     = "ANY"
  }
}
