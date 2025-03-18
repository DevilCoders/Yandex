#!/bin/sh
ya m > /dev/null && ./furniture_factory run FurnitureFactoryPy --input "`cat input.example.json`" --test
