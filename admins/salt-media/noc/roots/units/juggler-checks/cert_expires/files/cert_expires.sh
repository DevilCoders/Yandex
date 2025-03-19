#!/bin/bash
echo "PASSIVE-CHECK:cert_expires;"$(/usr/lib/config-monrun-cert-check/cert_expires.sh $*)
