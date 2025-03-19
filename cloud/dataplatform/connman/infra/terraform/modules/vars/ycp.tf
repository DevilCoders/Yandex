locals {
  ycp = {
    "preprod" : local.ycp_preprod
    "prod" : local.ycp_prod
  }
  ycp_preprod = {
    prod : false
  }
  ycp_prod = {
    prod : true
  }
}