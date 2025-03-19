#!/bin/bash

dbname=$1
from=$2
to=$3
uniq=$(date +%Y-%M-%d-%H-%m)
user=root

echo $to | grep .dev ||  echo "You are going copy something with this script not to dev. Are you sure?"; read

ssh ${user}@${from} mysqldump $dbname > $dbname-$uniq.sql
ssh ${user}@${to} mysqldump $dbname > $dbname-$uniq-bak.sql

ssh ${user}@${to} mysqladmin drop $dbname

ssh ${user}@${to} mysqladmin create $dbname

ssh ${user}@${to} mysql $dbname < $dbname-$uniq.sql


