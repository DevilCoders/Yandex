module "auth_ui" {
  source                 = "../../../modules/infra/auth_ui/v1"

  infra_name             = var.infra_name
  auth_ui_repository_arn = aws_ecr_repository.auth-ui.arn
  auth_ui_devops         = var.auth_ui_devops
}