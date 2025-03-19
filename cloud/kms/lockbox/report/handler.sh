IAM_TOKEN=`curl -H'Metadata-Flavor:Google' \
'http://169.254.169.254/computeMetadata/v1/instance/service-accounts/default/token' | jq -r ."access_token"`

YQL_OAUTH_TOKEN=`curl -XGET -H'Authorization: Bearer '$IAM_TOKEN \
'https://payload.lockbox.api.cloud-preprod.yandex.net/lockbox/v1/secrets/fc3jhcpk7bsc39mfev51/payload' | jq -r '."entries"[] | select(."key"=="token")."textValue"'`

REQUEST=`echo '{
  "action": "RUN",
  "type": "SQLv1"
}' | jq --rawfile query 'query.yql' '. + {"content": $query}'`
OUTPUT=`curl -H"Authorization: OAuth $YQL_OAUTH_TOKEN" 'https://yql.yandex.net/api/v2/operations' -d"$REQUEST"`

echo '{
    "statusCode": 200,
    "body": '$OUTPUT'
}'
