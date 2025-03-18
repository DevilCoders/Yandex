#!/usr/bin/env python

import sys
from app import app
from werkzeug.debug import DebuggedApplication

if __name__ == "__main__":
    assert len(sys.argv) < 3
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 25000
    app.run(debug=True, host='::', port=port, processes=4)
else:
    app.debug = True
    app = DebuggedApplication(app, evalex=False, pin_security=False, pin_logging=False)
