resource "ycp_vpc_network" "mdb_controlplane_dualstacknets-nets" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id = var.folder_id
  name      = "mdb-controlplane-dualstack-nets"
}

resource "ycp_vpc_subnet" "mdb_controlplane_dualstacknets-ru-central1-a" {
  lifecycle {
    prevent_destroy = true
  }
  v6_cidr_blocks = ["2a02:6b8:c0e:501:0:fc49::/112"]
  v4_cidr_blocks = ["10.1.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.folder_id
  name       = "mdb-controlplane-dualstack-nets-ru-central1-a"
  network_id = ycp_vpc_network.mdb_controlplane_dualstacknets-nets.id
  zone_id    = "ru-central1-a"
}

resource "ycp_vpc_subnet" "mdb_controlplane_dualstacknets-ru-central1-b" {
  lifecycle {
    prevent_destroy = false
  }
  v6_cidr_blocks = ["2a02:6b8:c02:901:0:fc49::/112"]
  v4_cidr_blocks = ["10.2.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.folder_id
  name       = "mdb-controlplane-dualstack-nets-ru-central1-b"
  network_id = ycp_vpc_network.mdb_controlplane_dualstacknets-nets.id
  zone_id    = "ru-central1-b"
}

resource "ycp_vpc_subnet" "mdb_controlplane_dualstacknets-ru-central1-c" {
  lifecycle {
    prevent_destroy = false
  }
  v6_cidr_blocks = ["2a02:6b8:c03:501:0:fc49::/112"]
  v4_cidr_blocks = ["10.4.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.folder_id
  name       = "mdb-controlplane-dualstack-nets-ru-central1-c"
  network_id = ycp_vpc_network.mdb_controlplane_dualstacknets-nets.id
  zone_id    = "ru-central1-c"
}
