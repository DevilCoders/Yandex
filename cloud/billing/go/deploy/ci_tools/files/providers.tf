terraform {
  required_providers {
    yandex = {
      source = "terraform-registry.storage.yandexcloud.net/yandex-cloud/yandex"
    }
    external = {
      source  = "terraform-registry.storage.yandexcloud.net/hashicorp/external"
      version = "2.2.0"
    }
    null = {
      source  = "terraform-registry.storage.yandexcloud.net/hashicorp/null"
      version = "3.1.0"
    }
    template = {
      source  = "terraform-registry.storage.yandexcloud.net/hashicorp/template"
      version = "2.2.0"
    }
  }
}
