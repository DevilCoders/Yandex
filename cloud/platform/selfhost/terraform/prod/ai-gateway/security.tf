# folder: b1ggokjh96von6jdu14e
# kotiki: b1g5a3pvuu2lm24l443c
# common: b1g9p9sqa7m6mv6uqsre

# yandex_kms_symmetric_key.gateway_kek abjqclq1020l42u5e46m
resource "yandex_kms_symmetric_key" gateway_kek {
#  id                = "abjqclq1020l42u5e46m"
  name              = "ai-gateway-kek"
  description       = "KEK for encrypting/decrypting ai-gateway secrets"
  default_algorithm = "AES_256"
}

# Bindings
#
# yc kms symmetric-key add-access-binding --id abjqclq1020l42u5e46m --role kms.keys.encrypterDecrypter --subject serviceAccount:aje9rdl372boefeaqk5l --profile prod
#
# yandex_iam_service_account.gateway_instance_sa aje9rdl372boefeaqk5l
resource "yandex_iam_service_account" gateway_instance_sa {
#  id                = "aje9rdl372boefeaqk5l"
  name              = "ai-gateway-instance-sa"
  description       = "Service Account for ai-gateway instances"
}

# Bindings
#
# folder: yc resource-manager folder add-access-binding --id b1ggokjh96von6jdu14e --role editor --subject serviceAccount:ajeedm805gjuto10be1r --profile prod
# kotiki: yc resource-manager folder add-access-binding --id b1g5a3pvuu2lm24l443c --role editor --subject serviceAccount:ajeedm805gjuto10be1r --profile prod
# common: yc resource-manager folder add-access-binding --id b1g9p9sqa7m6mv6uqsre --role editor --subject serviceAccount:ajeedm805gjuto10be1r --profile prod
#
# yandex_iam_service_account.gateway_ig_sa ajeedm805gjuto10be1r
resource "yandex_iam_service_account" gateway_ig_sa {
#  id                = "ajeedm805gjuto10be1r"
  name              = "ai-gateway-ig-sa"
  description       = "Service account for ai-gateway instance group management"
}

# Docker puller sa is in prod/ai-gateway/security.tf
# Packer sa is in prod/ai-gateway/security.tf
