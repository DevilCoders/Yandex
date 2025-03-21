
terraform {
  required_version = ">= 0.13"
  required_providers {
    random = {
      source = "hashicorp/random"
    }
    # template = {
    #   source = "hashicorp/template"
    # }
    yandex = {
      source = "yandex-cloud/yandex"
    }
  }
}
