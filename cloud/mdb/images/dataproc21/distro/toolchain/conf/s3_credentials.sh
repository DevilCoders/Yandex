export AWS_ACCESS_KEY_ID=$(cat /ws/secrets/AWS_ACCESS_KEY_ID)
export AWS_SECRET_ACCESS_KEY=$(cat /ws/secrets/AWS_SECRET_ACCESS_KEY)
export S3_BUCKET=$(cat /ws/secrets/BUCKET)
export S3_BUCKET_PREFIX=ci/$(cat /ws/secrets/BRANCH)/$(cat /ws/secrets/REVISION)
