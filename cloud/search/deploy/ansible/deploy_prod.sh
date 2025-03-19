#!/bin/bash
ansible-playbook -i ./inventory-yc-prod.yaml -u ubuntu ./site.yml $@
