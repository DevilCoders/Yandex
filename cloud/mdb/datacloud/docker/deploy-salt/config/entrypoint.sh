#!/bin/bash

# This is needed for cron job
echo "AWS_ACCESS_KEY=${AWS_ACCESS_KEY}" >> /etc/environment
echo "AWS_SECRET_KEY=${AWS_SECRET_KEY}" >> /etc/environment

/usr/bin/supervisord -c /etc/supervisor/supervisord.conf
