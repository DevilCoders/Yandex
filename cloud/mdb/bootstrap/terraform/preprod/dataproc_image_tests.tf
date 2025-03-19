resource "ycp_vpc_network" "mdb_dataproctest_dualstacknets-nets" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id = var.dataproc_image_test_folder_id
  name      = "mdb-dataproctest-dualstack-nets"
}

resource "ycp_vpc_subnet" "mdb_dataproctest_dualstacknets-ru-central1-a" {
  lifecycle {
    prevent_destroy = true
  }
  v6_cidr_blocks = ["2a02:6b8:c0e:501:0:fc51::/112"]
  v4_cidr_blocks = ["10.8.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.dataproc_image_test_folder_id
  name       = "mdb-dataproctest-dualstack-nets-ru-central1-a"
  network_id = ycp_vpc_network.mdb_dataproctest_dualstacknets-nets.id
  zone_id    = "ru-central1-a"
}

resource "ycp_vpc_subnet" "mdb_dataproctest_dualstacknets-ru-central1-b" {
  lifecycle {
    prevent_destroy = false
  }
  v6_cidr_blocks = ["2a02:6b8:c02:901:0:fc51::/112"]
  v4_cidr_blocks = ["10.9.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.dataproc_image_test_folder_id
  name       = "mdb-dataproctest-dualstack-nets-ru-central1-b"
  network_id = ycp_vpc_network.mdb_dataproctest_dualstacknets-nets.id
  zone_id    = "ru-central1-b"
}

resource "ycp_vpc_subnet" "mdb_dataproctest_dualstacknets-ru-central1-c" {
  lifecycle {
    prevent_destroy = false
  }
  v6_cidr_blocks = ["2a02:6b8:c03:501:0:fc51::/112"]
  v4_cidr_blocks = ["10.10.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.dataproc_image_test_folder_id
  name       = "mdb-dataproctest-dualstack-nets-ru-central1-c"
  network_id = ycp_vpc_network.mdb_dataproctest_dualstacknets-nets.id
  zone_id    = "ru-central1-c"
}

resource "yandex_vpc_security_group" "dataproc-tests-sgs" {
  name        = "dataproc-tests-sgs"
  description = "datap proc test dual stack network"
  network_id  = ycp_vpc_network.mdb_dataproctest_dualstacknets-nets.id
  folder_id   = var.dataproc_image_test_folder_id

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
}
