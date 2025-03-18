#!/bin/sh
ya m > /dev/null && ./picklock run Picklock --input "`cat input.example.json`" --test
