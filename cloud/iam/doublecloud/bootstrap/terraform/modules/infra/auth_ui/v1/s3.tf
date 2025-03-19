resource "aws_s3_bucket" "auth_ui_tf_state" {
  bucket = "datacloud-auth-ui-${var.infra_name}-tfstate"
  acl = "private"

  versioning {
    enabled = true
  }
}

resource "aws_s3_bucket_public_access_block" "auth_ui_tf_state" {
  bucket = aws_s3_bucket.auth_ui_tf_state.id

  block_public_acls       = true
  block_public_policy     = true
  ignore_public_acls      = true
  restrict_public_buckets = true
}

resource "aws_iam_policy" "auth_ui_ManageS3Bucket" {
  name = "${var.infra_name}_auth_ui_ManageS3Bucket"

  policy = <<POLICY
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "VisualEditor0",
            "Effect": "Allow",
            "Action": [
                "s3:DeleteObject",
                "s3:GetObject",
                "s3:ListBucket",
                "s3:PutObject"
            ],
            "Resource": [
                "${aws_s3_bucket.auth_ui_tf_state.arn}",
                "${aws_s3_bucket.auth_ui_tf_state.arn}/*"
            ]
        }
    ]
}
POLICY
}

resource "aws_iam_policy_attachment" "auth_ui_ManageS3Bucket" {
  name       = "${var.infra_name}_auth_ui_ManageS3Bucket"
  users      = var.auth_ui_devops
  policy_arn = aws_iam_policy.auth_ui_ManageS3Bucket.arn
}
