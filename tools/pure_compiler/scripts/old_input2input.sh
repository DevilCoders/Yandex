#!/bin/sh

. ./error_functions.sh

RunAndKill python rearrange_input.py | 
    LC_ALL=C sort
