data "ytr_conductor_project" "cloud" {
  name = "cloud"
}

data "ytr_conductor_datacenter" "vla" {
  name = "vla"
}

data "ytr_conductor_datacenter" "sas" {
  name = "sas"
}

data "ytr_conductor_datacenter" "myt" {
  name = "myt"
}

data "ytr_conductor_group" "mr_prober" {
  name = var.mr_prober_conductor_group_name
}

locals {
  ytr_conductor_datacenter_by_datacenter = {
    vla = data.ytr_conductor_datacenter.vla.id
    sas = data.ytr_conductor_datacenter.sas.id
    myt = data.ytr_conductor_datacenter.myt.id
  }

  ytr_conductor_datacenter_by_zone = {
    ru-central1-a = data.ytr_conductor_datacenter.vla.id
    ru-central1-b = data.ytr_conductor_datacenter.sas.id
    ru-central1-c = data.ytr_conductor_datacenter.myt.id
  }
}

resource "ytr_conductor_group" "cluster" {
  name = "cloud_${var.label_environment}_mr_prober_${var.prefix}"
  project_id = data.ytr_conductor_project.cloud.id
  description = "${var.prefix} cluster, part of Mr. Prober"
  parent_group_ids = [
    data.ytr_conductor_group.mr_prober.id
  ]
}
