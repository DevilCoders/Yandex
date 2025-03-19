#!/bin/bash

message=$(check | tr "\r\n" ",")

if [[ $message == "" ]]; then 
    echo "0; all servants seems up"
else echo "2; $message"
fi

