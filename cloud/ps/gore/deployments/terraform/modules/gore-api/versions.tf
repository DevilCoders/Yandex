terraform {
  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }

    ytr = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ytr"
    }

    yandex = {
      source = "terraform-registry.storage.yandexcloud.net/yandex-cloud/yandex"
    }

    random = {
      source = "terraform-registry.storage.yandexcloud.net/hashicorp/random"
    }    
  }
}
