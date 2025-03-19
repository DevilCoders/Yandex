DEFAULT_EC2_ASSUME_ROLE_POLICY = '''{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "",
            "Effect": "Allow",
            "Principal": {
                "Service": "ec2.amazonaws.com"
            },
            "Action": "sts:AssumeRole"
        }
    ]
}'''


DEFAULT_BYOA_CLUSTER_ROLE_POLICY = '''{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "AccessToClusterS3Buckets",
            "Effect": "Allow",
            "Resource": [
                "arn:aws:s3:::double-cloud-*",
                "arn:aws:s3:::double-cloud-*/*"
            ],
            "Action": "s3:*"
        },
        {
            "Sid": "AccessToDebRepos",
            "Effect": "Allow",
            "Resource": [
                "arn:aws:s3:::mdb-*-stable-*",
                "arn:aws:s3:::mdb-*-stable-*/*"
            ],
            "Action": [
                "s3:GetObject",
                "s3:ListBucket"
            ]
        },
        {
            "Sid": "AllowAssumeAnyRole",
            "Effect": "Allow",
            "Resource": "*",
            "Action": [
                "sts:AssumeRole"
            ]
        }
    ]
}'''

BYOA_ASSUME_ROLE_POLICY_TEMPLATE = '''{{
    "Version": "2012-10-17",
    "Statement": [
        {{
            "Sid": "",
            "Effect": "Allow",
            "Principal": {{
                "AWS": "{role_arn}"
            }},
            "Action": "sts:AssumeRole"
        }}
    ]
}}'''
