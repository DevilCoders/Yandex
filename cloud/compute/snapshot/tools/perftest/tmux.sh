#!/bin/bash

sleep="sleep 0.5"

tmux new-session -d; $sleep
tmux rename-window "test"
tmux send-keys "./test_restore.sh | tee test.log; ./parse_restore.py test.log"

tmux new-window -n atop; $sleep
tmux send-keys "sudo atop 1" Enter

tmux new-window -n journalctl; $sleep
tmux send-keys "sudo journalctl -u yc-snapshot -f" Enter; $sleep

tmux select-window -t 0
tmux attach
