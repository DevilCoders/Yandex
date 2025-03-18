#!/usr/bin/env bash
ssh-agent bash -c "ssh-add web_shared/robot_git && web_shared/loop.py wbe -p 7200 --no-auth"

