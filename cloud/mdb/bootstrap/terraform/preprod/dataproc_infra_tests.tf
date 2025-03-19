resource "ycp_vpc_network" "mdb_dataproc_infra_test_dualstack_nets" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id = var.dataproc_infra_test_folder_id
  name      = "mdb-dataproc-infra-test-dualstack-nets"
}

resource "ycp_vpc_subnet" "mdb_dataproc_infra_test_dualstack_nets-ru-central1-a" {
  lifecycle {
    prevent_destroy = true
  }
  v6_cidr_blocks = ["2a02:6b8:c0e:501:0:fc64::/112"]
  v4_cidr_blocks = ["10.11.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.dataproc_infra_test_folder_id
  name       = "mdb-dataproc-infra-test-dualstack-nets-ru-central1-a"
  network_id = ycp_vpc_network.mdb_dataproc_infra_test_dualstack_nets.id
  zone_id    = "ru-central1-a"
}

resource "ycp_vpc_subnet" "mdb_dataproc_infra_test_dualstacknets-nets-ru-central1-b" {
  lifecycle {
    prevent_destroy = false
  }
  v6_cidr_blocks = ["2a02:6b8:c02:901:0:fc64::/112"]
  v4_cidr_blocks = ["10.12.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.dataproc_infra_test_folder_id
  name       = "mdb-dataproc-infra-test-dualstack-nets-ru-central1-b"
  network_id = ycp_vpc_network.mdb_dataproc_infra_test_dualstack_nets.id
  zone_id    = "ru-central1-b"
}

resource "ycp_vpc_subnet" "mdb_dataproc_infra_test_dualstack_nets-ru-central1-c" {
  lifecycle {
    prevent_destroy = false
  }
  v6_cidr_blocks = ["2a02:6b8:c03:501:0:fc64::/112"]
  v4_cidr_blocks = ["10.13.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.dataproc_infra_test_folder_id
  name       = "mdb-dataproc-infra-test-dualstack-nets-ru-central1-c"
  network_id = ycp_vpc_network.mdb_dataproc_infra_test_dualstack_nets.id
  zone_id    = "ru-central1-c"
}

resource "yandex_vpc_security_group" "dataproc-infra-tests-sgs" {
  name        = "dataproc-infra-tests-sgs"
  description = "MDB-11517: dataproc security group"
  network_id  = ycp_vpc_network.mdb_dataproc_infra_test_dualstack_nets.id
  folder_id   = var.dataproc_infra_test_folder_id

  ingress {
    protocol       = "ICMP"
    description    = "Developers and CI uses icmp for debugging"
    v4_cidr_blocks = ["10.0.0.0/8"] # local network
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }
  ingress {
    protocol       = "TCP"
    description    = "allow SSH access"
    v4_cidr_blocks = ["10.0.0.0/8"] # local network
    v6_cidr_blocks = local.v6-yandex-and-cloud
    port           = 22
  }

  ingress {
    protocol          = "TCP"
    description       = "allow local-network traffic"
    predefined_target = "self_security_group"
    from_port         = 0
    to_port           = 65535
  }

  ingress {
    description    = "kafka"
    from_port      = 9091
    labels         = {}
    port           = 65535
    protocol       = "TCP"
    to_port        = 9092
    v4_cidr_blocks = ["10.0.0.0/8"]
    v6_cidr_blocks = []
  }

  ingress {
    description    = "Kafka"
    from_port      = 9091
    labels         = {}
    port           = 65535
    protocol       = "ANY"
    to_port        = 9092
    v4_cidr_blocks = ["192.168.0.0/16"]
    v6_cidr_blocks = []
  }

  egress {
    protocol          = "ANY"
    description       = "Self EGRESS"
    predefined_target = "self_security_group"
    from_port         = 0
    to_port           = 65535
  }

  egress {
    protocol       = "ANY"
    description    = "to external endpoints (s3, kms, lockbox, mvn)"
    from_port      = 0
    to_port        = 65535
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
  }

  egress {
    description    = "dataproc manager"
    from_port      = 65535
    labels         = {}
    port           = 11003
    protocol       = "TCP"
    to_port        = 65535
    v4_cidr_blocks = ["192.168.0.0/16"]
    v6_cidr_blocks = []
  }

  egress {
    description    = "monitoring"
    from_port      = 65535
    labels         = {}
    port           = 443
    protocol       = "TCP"
    to_port        = 65535
    v4_cidr_blocks = ["87.250.250.10/32"]
    v6_cidr_blocks = []
  }

  egress {
    description    = "s3 prod"
    from_port      = 65535
    labels         = {}
    port           = 443
    protocol       = "TCP"
    to_port        = 65535
    v4_cidr_blocks = ["213.180.193.243/32"]
    v6_cidr_blocks = []
  }

  egress {
    description    = "UI Proxy"
    from_port      = 65535
    labels         = {}
    port           = 2181
    protocol       = "TCP"
    to_port        = 65535
    v4_cidr_blocks = ["192.168.0.0/16"]
    v6_cidr_blocks = []
  }

}


resource "ycp_vpc_network" "dataproc_infratest_client_net" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id = var.dataproc_infra_test_folder_id
  name      = "dataproc-infratest-client-net"
}

resource "ycp_vpc_subnet" "dataproc_infratest_client_net-ru-central1-a" {
  lifecycle {
    prevent_destroy = true
  }
  v6_cidr_blocks = ["2a02:6b8:c0e:501:0:fc6e::/112"]
  v4_cidr_blocks = ["10.11.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.dataproc_infra_test_folder_id
  name       = "dataproc-infratest-client-net-ru-central1-a"
  network_id = ycp_vpc_network.dataproc_infratest_client_net.id
  zone_id    = "ru-central1-a"
}

resource "ycp_vpc_subnet" "dataproc_infratest_client_net-ru-central1-b" {
  lifecycle {
    prevent_destroy = false
  }
  v6_cidr_blocks = ["2a02:6b8:c02:901:0:fc6e::/112"]
  v4_cidr_blocks = ["10.12.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.dataproc_infra_test_folder_id
  name       = "dataproc-infratest-client-net-ru-central1-b"
  network_id = ycp_vpc_network.dataproc_infratest_client_net.id
  zone_id    = "ru-central1-b"
}

resource "ycp_vpc_subnet" "dataproc_infratest_client_net-ru-central1-c" {
  lifecycle {
    prevent_destroy = false
  }
  v6_cidr_blocks = ["2a02:6b8:c03:501:0:fc6e::/112"]
  v4_cidr_blocks = ["10.13.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.dataproc_infra_test_folder_id
  name       = "dataproc-infratest-client-net-ru-central1-c"
  network_id = ycp_vpc_network.dataproc_infratest_client_net.id
  zone_id    = "ru-central1-c"
}

resource "yandex_vpc_security_group" "kafka-infratest-sg" {
  name        = "kafka-infratest-sg"
  description = "user-SG that is applied to infratest Kafka clusters"
  network_id  = ycp_vpc_network.dataproc_infratest_client_net.id
  folder_id   = var.dataproc_infra_test_folder_id

  ingress {
    protocol       = "TCP"
    description    = "connect to Kafka brokers from developer laptops and from Jenkins"
    v6_cidr_blocks = local.v6-yandex-and-cloud
    from_port      = 9091
    to_port        = 9092
  }

  ingress {
    protocol       = "TCP"
    description    = "allow traffic between two Kafka clusters in order to use MirrorMaker connector"
    v4_cidr_blocks = ["10.0.0.0/8"] # local network
    from_port      = 9091
    to_port        = 9092
  }

  egress {
    protocol       = "TCP"
    description    = "allow traffic between two Kafka clusters in order to use MirrorMaker connector"
    v4_cidr_blocks = ["10.0.0.0/8"] # local network
    from_port      = 9091
    to_port        = 9092
  }

  # Temporary rule until fixed within https://st.yandex-team.ru/CLOUD-83561
  egress {
    protocol       = "TCP"
    description    = "allow tcp traffic to ipv6 cloud dns servers, until fixed within https://st.yandex-team.ru/CLOUD-83561"
    v6_cidr_blocks = local.v6-yandex-and-cloud
    port           = 53
  }
}

resource "ycp_vpc_network" "mdb_sqlserver_infra_test_nets" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id = var.dataproc_infra_test_folder_id
  name      = "mdb-sqlserver-infra-test-nets"
}

resource "ycp_vpc_subnet" "mdb_sqlserver_infra_test_nets_ipv4_only_ru_central1_b" {
  lifecycle {
    prevent_destroy = true
  }
  v4_cidr_blocks = ["10.15.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.dataproc_infra_test_folder_id
  name       = "mdb-sqlserver-infra-test-nets-ipv4-only-ru-central1-b"
  network_id = ycp_vpc_network.mdb_sqlserver_infra_test_nets.id
  zone_id    = "ru-central1-b"
}

resource "ycp_vpc_subnet" "mdb_sqlserver_infra_test_nets_ipv4_only_ru_central1_a" {
  lifecycle {
    prevent_destroy = true
  }
  v4_cidr_blocks = ["10.11.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.dataproc_infra_test_folder_id
  name       = "mdb-sqlserver-infra-test-nets-ipv4-only-ru-central1-a"
  network_id = ycp_vpc_network.mdb_sqlserver_infra_test_nets.id
  zone_id    = "ru-central1-a"
}

resource "ycp_vpc_subnet" "mdb_sqlserver_infra_test_nets_ipv4_only_ru_central1_c" {
  lifecycle {
    prevent_destroy = true
  }
  v4_cidr_blocks = ["10.13.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.dataproc_infra_test_folder_id
  name       = "mdb-sqlserver-infra-test-nets-ipv4-only-ru-central1-c"
  network_id = ycp_vpc_network.mdb_sqlserver_infra_test_nets.id
  zone_id    = "ru-central1-c"
}
