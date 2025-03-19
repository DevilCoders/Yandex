#!/usr/bin/env bash
docker exec support-queue bash -c "python3 /src/support-queue/yc_support_queue/spawner.py"
