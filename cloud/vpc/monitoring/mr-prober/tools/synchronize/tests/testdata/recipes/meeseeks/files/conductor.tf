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

locals {
  ytr_conductor_datacenter_by_datacenter = {
    vla = data.ytr_conductor_datacenter.vla.id
    sas = data.ytr_conductor_datacenter.sas.id
    myt = data.ytr_conductor_datacenter.myt.id
  }
}

resource "ytr_conductor_group" "meeseeks" {
  name             = "cloud_${var.label_environment}_mr_prober_meeseeks"
  project_id       = data.ytr_conductor_project.cloud.id
  description      = "Meeseeks cluster, part of Mr. Prober"
  parent_group_ids = [
    var.mr_prober_conductor_group_id
  ]
}

resource "ytr_conductor_group" "meeseeks_in_datacenter" {
  for_each         = ["vla", "sas", "myt"]
  name             = "cloud_${var.label_environment}_mr_prober_meeseeks_${each.key}"
  project_id       = data.ytr_conductor_project.cloud.id
  description      = "Meeseeks cluster in ${each.key}, part of Mr. Prober"
  parent_group_ids = [
    ytr_conductor_group.meeseeks.id
  ]
}
