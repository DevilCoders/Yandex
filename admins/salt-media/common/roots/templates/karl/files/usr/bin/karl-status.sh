#!/bin/bash

status=`sudo karl-cli ping --tls 2>/dev/null`
code=$?

if [ $code -eq 0 ]
then
	echo '0; Ok'
else
	echo "2; Err code: $code"
fi
