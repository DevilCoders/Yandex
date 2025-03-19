#!/bin/bash
set -e
FILE=$1;
service=$2;
profile="prod";
secret_name=$(basename "$FILE");
file_name=$secret_name;
echo "secret name is $secret_name";
echo "secret file name is $file_name";

secret_id=$(yc-secret-cli --profile $profile secret add --name $secret_name --file $FILE | jq -r '.id');
echo "secret id is $secret_id";

yc-secret-cli --profile $profile secret set $secret_id --version 1 --filename $file_name --group $service --user $service;
yc-secret-cli --profile $profile secret grant $secret_id --usergroup abc_yc_iam_dev --role admin
yc-secret-cli --profile $profile hostgroup add-secret bootstrap_base-role_prod_iam-ya --secret-id $secret_id;


echo "check this secret on iam-svm in path /usr/share/yc-secrets/$secret_id/$secret_name/1/$file_name";

