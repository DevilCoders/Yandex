module "monops_sa" {
  source = "../../modules/tools/monops/viewer_sa/"

  // yc-tools/vpc on testing
  folder_id = "yc.tools.vpc"
}

