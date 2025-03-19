#!/bin/bash
#NOCDEV-3953

mysync maint get 2>&1 | grep -q on && echo "PASSIVE-CHECK:mysync-maint;CRIT;maintenance enable" || echo "PASSIVE-CHECK:mysync-maint;OK;OK, maintenance disable"
