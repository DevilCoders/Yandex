#!/usr/bin/env bash

set -e -o pipefail

. venv/bin/activate

export PYTHONPATH=ui/pyhtml_snapshot:$PYTHONPATH
python ui/assessor_ui/serve.py

