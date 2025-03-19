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
  ytr_conductor_datacenter_by_zone = {
    ru-central1-a = data.ytr_conductor_datacenter.vla.id
    ru-central1-b = data.ytr_conductor_datacenter.sas.id
    ru-central1-c = data.ytr_conductor_datacenter.myt.id
  }
}

resource "ytr_conductor_group" "mr_prober" {
  count = var.use_conductor ? 1 : 0
  name = "cloud_${var.mr_prober_environment}_mr_prober"
  project_id = data.ytr_conductor_project.cloud.id
  description = "All instances of Mr. Prober"
}

resource "ytr_conductor_group" "mr_prober_clusters" {
  count = var.use_conductor ? 1 : 0
  name = "cloud_${var.mr_prober_environment}_mr_prober_clusters"
  project_id = data.ytr_conductor_project.cloud.id
  description = "All clusters of Mr. Prober"
  parent_group_ids = [
    ytr_conductor_group.mr_prober[0].id
  ]
}
