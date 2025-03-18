#!/bin/sh
ya m > /dev/null && ./woodcutter run WoodcutterPy --input "`cat input.example.json`" --test
