resource "ycp_vpc_network" "cloud-mdb-gpn-appendix-prod-nets" {
  lifecycle {
    prevent_destroy = true
  }
  name = "cloud-mdb-gpn-appendix-prod-nets"
}

resource "ycp_vpc_subnet" "cloud-mdb-gpn-appendix-prod-nets-ru-central1-a" {
  lifecycle {
    prevent_destroy = true
  }

  v4_cidr_blocks = ["172.16.0.0/16"]
  v6_cidr_blocks = ["2a02:6b8:c0e:500:0:4f26::/96"] # _CLOUD_MDB_GPN_APPENDIX_PROD_NETS_
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  folder_id  = var.folder_id
  name       = "cloud-mdb-gpn-appendix-prod-nets-ru-central1-a"
  network_id = ycp_vpc_network.cloud-mdb-gpn-appendix-prod-nets.id
  zone_id    = "ru-central1-a"
}

resource "ycp_vpc_subnet" "cloud-mdb-gpn-appendix-prod-nets-ru-central1-b" {
  lifecycle {
    prevent_destroy = true
  }

  v6_cidr_blocks = ["2a02:6b8:c02:900:0:4f26::/96"] # _CLOUD_MDB_GPN_APPENDIX_PROD_NETS_
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  folder_id  = var.folder_id
  name       = "cloud-mdb-gpn-appendix-prod-nets-ru-central1-b"
  network_id = ycp_vpc_network.cloud-mdb-gpn-appendix-prod-nets.id
  zone_id    = "ru-central1-b"
}

//resource "ycp_vpc_subnet" "cloud-mdb-gpn-appendix-prod-nets-ru-central1-c" {
//    lifecycle {
//        prevent_destroy = true
//    }
//
//    v6_cidr_blocks = ["2a02:6b8:c03:500:0:4f26::/96"] # _CLOUD_MDB_GPN_APPENDIX_PROD_NETS_
//    extra_params {
//        export_rts  = ["65533:666"]
//        hbf_enabled = true
//        import_rts  = ["65533:776"]
//        rpf_enabled = false
//    }
//    folder_id  = var.folder_id
//    name       = "cloud-mdb-gpn-appendix-prod-nets-ru-central1-c"
//    network_id = ycp_vpc_network.mdb-appendix-cp-nets.id
//    zone_id    = "ru-central1-c"
//}

data "yandex_vpc_network" "cloud-prod-dmzprivcloud-nets" {
  network_id = "enpb8i5soo379t54lom2"
}

data "yandex_vpc_subnet" "interconnect-nets-ru-central1-a" {
  subnet_id = "e9bheh9m8mtc8i8m0gbv"
}

data "yandex_vpc_subnet" "interconnect-nets-ru-central1-b" {
  subnet_id = "e2l34dilta6fme6kn10k"
}
