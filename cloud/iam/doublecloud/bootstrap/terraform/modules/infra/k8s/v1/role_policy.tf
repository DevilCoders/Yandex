resource "aws_iam_role_policy" "iam_k8s_KMSEncryptDecryptPolicy" {
  name = "${var.cluster_name}-k8s-encrypt-decrypt"
  role = aws_iam_role.cluster.id

  policy = jsonencode({
    "Version" = "2012-10-17",
    "Statement" = [
      {
        Effect = "Allow",
        Action = [
          # Parsed from https://www.eksworkshop.com/beginner/191_secrets/ there is no other sources.
          "kms:Encrypt",
          "kms:Decrypt"
        ],
        Resource = "*"
      }
    ]
  })
}
