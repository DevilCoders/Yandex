data "ytr_conductor_project" "cloud" {
  count = var.use_conductor ? 1 : 0
  name = "cloud"
}

data "ytr_conductor_datacenter" "vla" {
  count = var.use_conductor ? 1 : 0
  name = "vla"
}

data "ytr_conductor_datacenter" "sas" {
  count = var.use_conductor ? 1 : 0
  name = "sas"
}

data "ytr_conductor_datacenter" "myt" {
  count = var.use_conductor ? 1 : 0
  name = "myt"
}

data "ytr_conductor_group" "mr_prober" {
  count = var.use_conductor ? 1 : 0
  name = var.mr_prober_conductor_group_name
}

locals {
  ytr_conductor_datacenter_by_datacenter = var.use_conductor ? {
    vla = data.ytr_conductor_datacenter.vla[0].id
    sas = data.ytr_conductor_datacenter.sas[0].id
    myt = data.ytr_conductor_datacenter.myt[0].id
  } : {}
}

resource "ytr_conductor_group" "meeseeks" {
  count = var.use_conductor ? 1 : 0
  name = "cloud_${var.label_environment}_mr_prober_${var.prefix}"
  project_id = data.ytr_conductor_project.cloud[0].id
  description = "Meeseeks cluster, part of Mr. Prober"
  parent_group_ids = [
    data.ytr_conductor_group.mr_prober[0].id
  ]
}

resource "ytr_conductor_group" "meeseeks_in_datacenter" {
  for_each = toset(var.use_conductor ? var.zones : [])
  name = "cloud_${var.label_environment}_mr_prober_${var.prefix}_${each.key}"
  project_id = data.ytr_conductor_project.cloud[0].id
  description = "Meeseeks cluster in ${each.key}, part of Mr. Prober"
  parent_group_ids = [
    ytr_conductor_group.meeseeks[0].id
  ]
}
