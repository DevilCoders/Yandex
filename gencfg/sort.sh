#!/usr/bin/env bash

cut -f 1,4-6 | sort | uniq -c | sort -nr | sort -k 2
