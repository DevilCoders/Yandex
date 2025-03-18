#!/usr/local/bin/bash

ps afx | grep $1 | grep -v grep | grep -v "kill.sh" | awk '{print $1}' | xargs kill -9
