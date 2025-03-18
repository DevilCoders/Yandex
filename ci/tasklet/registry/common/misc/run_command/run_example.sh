#!/bin/sh
# ya make > /dev/null && ./run_command run RunCommand --input "`cat input_example_2.json`" --local
# ya make > /dev/null && ./run_command run RunCommand --input "`cat input_example.json`" --sandbox-tasklet --sb-schema
# ya make > /dev/null && ./run_command run RunCommand --input "`cat input_example_3.json`" --sandbox-tasklet --sb-schema
# ya make > /dev/null && ./run_command run RunCommand --input "`cat input_example_4.json`" --sandbox-tasklet
# ya make > /dev/null && ./run_command run RunCommand --input "`cat input_example_5.json`" --sandbox-tasklet
# ya make > /dev/null && ./run_command run RunCommand --input "`cat input_example_6.json`" --local 2> /dev/null
ya make > /dev/null && ./run_command run RunCommand --input "`cat input_example_7.json`" --sandbox-tasklet
