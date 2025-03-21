terraform {
  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }

    ytr = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ytr"
    }

    yandex = {
      source = "yandex-cloud/yandex"
    }
    
    external = {
      source = "hashicorp/external"
    }
  }
}
