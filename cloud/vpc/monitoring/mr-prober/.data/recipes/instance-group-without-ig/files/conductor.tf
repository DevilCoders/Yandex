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
  ytr_conductor_datacenter_by_zone = {
    ru-central1-a = data.ytr_conductor_datacenter.vla.id
    ru-central1-b = data.ytr_conductor_datacenter.sas.id
    ru-central1-c = data.ytr_conductor_datacenter.myt.id
  }
}

resource "ytr_conductor_group" "agents" {
  count = var.use_conductor ? 1 : 0
  name = "cloud_${var.label_environment}_mr_prober_${var.prefix}"
  project_id = data.ytr_conductor_project.cloud.id
  description = "Cluster ${var.prefix}, part of Mr. Prober"
  parent_group_ids = [ 
    data.ytr_conductor_group.mr_prober.id
  ]
}

resource "ytr_conductor_group" "agents_in_datacenter" {
  for_each = toset(var.use_conductor ? values(var.zone_to_dc) : [])
  name = "cloud_${var.label_environment}_mr_prober_${var.prefix}_${each.key}"
  project_id = data.ytr_conductor_project.cloud.id
  description = "Cluster ${var.prefix} in ${each.key}, part of Mr. Prober"
  parent_group_ids = [
    ytr_conductor_group.agents[0].id
  ]
}