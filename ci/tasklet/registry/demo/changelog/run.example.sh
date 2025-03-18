#!/bin/sh
ya m > /dev/null && ./changelog run Changelog --input "`cat input.example.json`" --test
