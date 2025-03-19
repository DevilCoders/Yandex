# folder: aoekcnbhhbs7f609rhnv
# kotiki: aoe9saplig4pa3pjpi87
# common: aoe5k83dn6vak86d5a3i
# paas-images: aoe824hvnc67es4f8kqj

# yandex_kms_symmetric_key.gateway_kek e107hj8dm41bl15t1bb0
resource "yandex_kms_symmetric_key" gateway_kek {
#  id                = "e107hj8dm41bl15t1bb0"
  name              = "api-gateway-kek"
  description       = "KEK for encrypting/decrypting api-gateway secrets"
  default_algorithm = "AES_256"
}

# Bindings
#
# yc kms symmetric-key add-access-binding --id e107hj8dm41bl15t1bb0 --role kms.keys.encrypterDecrypter --subject serviceAccount:bfbc9jtqmf9n3mp11sfv --profile preprod
#
# ycp_iam_service_account.gateway_instance_sa bfbc9jtqmf9n3mp11sfv
resource "ycp_iam_service_account" gateway_instance_sa {
#  id                = "bfbc9jtqmf9n3mp11sfv"
  name              = "api-gateway-instance-sa"
  description       = "Service Account for api-gateway instances"
}

# Bindings
#
# folder: yc resource-manager folder add-access-binding --id aoekcnbhhbs7f609rhnv --role editor --subject serviceAccount:bfbeiq2i4fffk37s6qo1 --profile preprod
# kotiki: yc resource-manager folder add-access-binding --id aoe9saplig4pa3pjpi87 --role editor --subject serviceAccount:bfbeiq2i4fffk37s6qo1 --profile preprod
# common: yc resource-manager folder add-access-binding --id aoe5k83dn6vak86d5a3i --role editor --subject serviceAccount:bfbeiq2i4fffk37s6qo1 --profile preprod
#
# ycp_iam_service_account.gateway_ig_sa bfbeiq2i4fffk37s6qo1
#
resource "ycp_iam_service_account" gateway_ig_sa {
#  id                = "bfbeiq2i4fffk37s6qo1"
  name              = "api-gateway-ig-sa"
  description       = "Service account for api-gateway instance group management"
}

# Bindings
#
# kotiki: yc resource-manager folder add-access-binding --id aoe9saplig4pa3pjpi87 --role editor --subject serviceAccount:bfbhe1lkpl6hnmhtqi36 --profile preprod
# common: yc resource-manager folder add-access-binding --id aoe5k83dn6vak86d5a3i --role editor --subject serviceAccount:bfbhe1lkpl6hnmhtqi36 --profile preprod
# paas  : yc resource-manager folder add-access-binding --id aoe824hvnc67es4f8kqj --role viewer --subject serviceAccount:bfbhe1lkpl6hnmhtqi36 --profile preprod
#
# Key
# yc iam key create --service-account-id bfbhe1lkpl6hnmhtqi36 --output sa.json  --profile preprod
# https://yav.yandex-team.ru/secret/sec-01eapwsr4wgweqdn7nrbtvsz9z
#
# ycp_iam_service_account.gateway_packer_sa bfbhe1lkpl6hnmhtqi36
resource "ycp_iam_service_account" gateway_packer_sa {
#  id                = "bfbhe1lkpl6hnmhtqi36"
  name              = "api-gateway-packer-sa"
  description       = "Service account for to build and store api-gateway image in assembly-shop-kotiki folder"
}

# Docker puller sa is in prod/api-gateway/security.tf
