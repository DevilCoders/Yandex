#!/bin/sh

export SECRET_KEY=sec-01ekn7gqq5vyrr45kb7q2ryy39
cd `dirname $0`/..
exec ../../../../../../kms/scripts/deploy/terraform-init.sh
