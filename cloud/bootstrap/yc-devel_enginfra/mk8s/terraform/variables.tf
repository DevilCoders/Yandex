variable "token" {
  description = "Iam token for stand"
}

locals {
  folder_id = "b1g2po48ce7bu9v9stf9" #enginfra 
  working-nodes-groups = {
      regular = 6
      canary = 2
  }
}