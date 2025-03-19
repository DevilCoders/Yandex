#!/bin/bash

LANG=C ip -6 addr  | grep 1>/dev/null "scope global .*dynamic" && echo "2; found autocreated address here" || echo "0; ok"

