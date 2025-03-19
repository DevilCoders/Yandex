variable "ydb-plugin-version" {
  type    = string
  default = "1.20.0-ed35f06"
}

variable "lb-reader-version" {
  type    = string
  default = "v12912-86e8a72"
}

variable "lb-plugin-version" {
  type    = string
  default = "v8294-47a52f1"
}

terraform {
  required_providers {
    template = {
      source = "hashicorp/template"
    }
    yandex = {
      source = "yandex-cloud/yandex"
    }
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
  }
  required_version = ">= 0.13"
}
