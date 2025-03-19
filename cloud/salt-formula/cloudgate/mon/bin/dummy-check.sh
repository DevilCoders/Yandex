#!/bin/bash
#This file is used when you need to gracefully remove/rename some check
name=${1:-name_unspecified}
echo "PASSIVE-CHECK:$name;0; This is dummy check. Remove me after deployed totally. ${@:2}"
