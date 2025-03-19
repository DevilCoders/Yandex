# resource "yandex_kms_symmetric_key" "xds_kek" {
#   name              = "xds-kek"
#   description       = "KEK for encrypting/decrypting xds secrets"
#   default_algorithm = "AES_256"
# }

resource "ycp_iam_service_account" "xds_alb_private_api_sa" {
  # id = "d26tc6c42a8nbmf4q39l"
  name              = "xds-alb-private-api-sa"
  description       = "Used to access alb api"
}

# These SA is a part of ALB API service deployment.
# TODO(iceman): Move it to alb folder.
resource "ycp_iam_service_account" "ydb_alb_testing" {
  # id = "d2672acp465imfk5ajck"
  name              = "sa-ydb-alb-testing"
  description       = "Used in ALB API instances"
  folder_id         = "batmvm4btt1d2eidsuqf" # alb
}

resource "ycp_iam_service_account" "alb_api_ig_manager" {
  # id = "d26qkn0mc1l8da444p6q"
  name              = "alb-api-ig-manager-sa"
  description       = "Used to manage ALB API instance group"
  folder_id         = "batmvm4btt1d2eidsuqf" # alb
}