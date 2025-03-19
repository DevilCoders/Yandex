variable "team_registries_push_permissions" {
  type = map(list(string))
  default = {
    //SAs from Assembly workshop that are used to build docker images
    "compute" : ["serviceAccount:ajeqsbktfh721njanru2"],
    "iam" : ["serviceAccount:ajen6o48b4fcj9t66ev9"],
    "infra" : ["serviceAccount:ajevncsl5cctlrkou8ic"],
    "enginfra" : [
      "serviceAccount:ajesaa3pfb1utkjvqmi6", //VPC SA currently
      "serviceAccount:aje5lrs84r6ike86uhrr", //Selfhost SA
    ]
    "vpc" : ["serviceAccount:ajesaa3pfb1utkjvqmi6"],
    "ydb" : ["serviceAccount:ajeba79m1rjdbblk1n2p"],
    "nbs" : ["serviceAccount:ajevo4spmcj1a0r4du54"],
  }
}

variable "team_registries_pull_permissions" {
  type = map(list(string))
  default = {
    "compute" : [
      "serviceAccount:yc.bootstrap.cr-puller-testing",
      "serviceAccount:yc.bootstrap.cr-puller-preprod",
      "serviceAccount:yc.bootstrap.cr-puller-prod",
      "serviceAccount:yc.bootstrap.cr-puller-israel",
      "serviceAccount:ajesaa3pfb1utkjvqmi6",
      "serviceAccount:aje5lrs84r6ike86uhrr", # aw selfhost-builder
    ],
    "iam" : [
      "serviceAccount:yc.bootstrap.cr-puller-testing",
      "serviceAccount:yc.bootstrap.cr-puller-preprod",
      "serviceAccount:yc.bootstrap.cr-puller-prod",
      "serviceAccount:yc.bootstrap.cr-puller-israel",
      "serviceAccount:ajen6o48b4fcj9t66ev9",
      "serviceAccount:aje5lrs84r6ike86uhrr", # aw selfhost-builder
    ],
    "infra" : [
      "serviceAccount:yc.bootstrap.cr-puller-testing",
      "serviceAccount:yc.bootstrap.cr-puller-preprod",
      "serviceAccount:yc.bootstrap.cr-puller-prod",
      "serviceAccount:yc.bootstrap.cr-puller-israel",
      "serviceAccount:ajesaa3pfb1utkjvqmi6",
      "serviceAccount:aje5lrs84r6ike86uhrr", # aw selfhost-builder
    ],
    "enginfra" : [
      "serviceAccount:yc.bootstrap.cr-puller-testing",
      "serviceAccount:yc.bootstrap.cr-puller-preprod",
      "serviceAccount:yc.bootstrap.cr-puller-prod",
      "serviceAccount:yc.bootstrap.cr-puller-israel",
      "serviceAccount:ajesaa3pfb1utkjvqmi6",
      "serviceAccount:aje5lrs84r6ike86uhrr", # aw selfhost-builder
    ],
    "vpc" : [
      "serviceAccount:yc.bootstrap.cr-puller-testing",
      "serviceAccount:yc.bootstrap.cr-puller-preprod",
      "serviceAccount:yc.bootstrap.cr-puller-prod",
      "serviceAccount:yc.bootstrap.cr-puller-israel",
      "serviceAccount:ajesaa3pfb1utkjvqmi6",
      "serviceAccount:aje5lrs84r6ike86uhrr", # aw selfhost-builder
    ],
    "ydb" : [
      "serviceAccount:yc.bootstrap.cr-puller-testing",
      "serviceAccount:yc.bootstrap.cr-puller-preprod",
      "serviceAccount:yc.bootstrap.cr-puller-prod",
      "serviceAccount:yc.bootstrap.cr-puller-israel",
      "serviceAccount:ajesaa3pfb1utkjvqmi6",
      "serviceAccount:aje5lrs84r6ike86uhrr", # aw selfhost-builder
    ],
    "nbs" : [
      "serviceAccount:yc.bootstrap.cr-puller-testing",
      "serviceAccount:yc.bootstrap.cr-puller-preprod",
      "serviceAccount:yc.bootstrap.cr-puller-prod",
      "serviceAccount:yc.bootstrap.cr-puller-israel",
      "serviceAccount:ajesaa3pfb1utkjvqmi6",
      "serviceAccount:aje5lrs84r6ike86uhrr", # aw selfhost-builder
    ],
  }
}

variable "custom_registries_pull_permissions" {
  type = map(list(string))
  default = {
    // TODO CLOUD-92321 delete this bindings
    // cr.yandex/yc-bootstrap
    "crpfc9niqhgtkgjjmu7b" : [
      "serviceAccount:yc.bootstrap.cr-puller-testing",
      "serviceAccount:yc.bootstrap.cr-puller-preprod",
      "serviceAccount:yc.bootstrap.cr-puller-prod",
      "serviceAccount:yc.bootstrap.cr-puller-israel",
      "serviceAccount:ajesaa3pfb1utkjvqmi6",
    ],
  }
}
