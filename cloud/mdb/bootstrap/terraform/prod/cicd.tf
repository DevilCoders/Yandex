resource "yandex_kms_symmetric_key" "datacloud_aws" {
  name              = "datacloud-aws"
  folder_id         = "b1g6j9md67bn436unp13"
  default_algorithm = "AES_256"
  rotation_period   = "24h"
}

resource "yandex_kms_symmetric_key" "yandexcloud" {
  name              = "yandexcloud"
  folder_id         = "b1g6j9md67bn436unp13"
  default_algorithm = "AES_256"
  rotation_period   = "24h"
}
