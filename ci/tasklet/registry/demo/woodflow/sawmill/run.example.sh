#!/bin/sh
ya m > /dev/null && ./sawmill run SawmillPy --input "`cat input.example.json`" --test
