import os
import sys
import logging
from logging.handlers import RotatingFileHandler

from flask import Flask, redirect, request, render_template, make_response, Response, current_app

from antirobot.scripts.log_viewer.config import Config
import yt_cache
import yt_client


app = Flask(__name__, root_path=os.path.join(os.getcwd(), 'app'))
app.debug = True

app.config['MY_CONFIG'] = Config  # FIXME: use Config instead
app.config['SECRET_KEY'] = 'AGHJF676876ttYUYT1'

ytInst = yt_client.GetYtClient(Config.YT_PROXY, Config.YT_TOKEN)
cache = yt_cache.YTCache(Config.CLUSTER_ROOT, ytInst)

import view
import calc_redirects_view
import precalc_view


ytLogger = logging.getLogger("Yt")
ytLogger.setLevel(logging.DEBUG if Config.DEBUG else logging.ERROR)

from antirobot.scripts.utils import spravka2
import traceback
try:
    with open(Config.KEYS_FILE, "r") as inp:
        spravka2.init_keyring("\n".join(inp.readlines()))
    print >> sys.stderr, "Keys was successfully loaded"
except:
    traceback.print_exc()
try:
    with open(Config.SPRAVKA_KEY_FILE, "r") as inp:
        spravka2.init_spravka_key("\n".join(inp.readlines()))
    print >> sys.stderr, "Spravka key was successfully loaded"
except:
    traceback.print_exc()



if os.path.exists(Config.LOG_DIR):
    fh = RotatingFileHandler(os.path.join(Config.LOG_DIR, 'yt.log'), maxBytes=10000000, backupCount=5)
    fh.setLevel(logging.DEBUG)
    fh.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s', datefmt='%Y-%m-%d %H:%M:%S'))

    ch = logging.StreamHandler()
    ch.setLevel(logging.ERROR)

    ytLogger.handlers[0].setLevel(logging.INFO) # exclude debug messages from console out
    ytLogger.addHandler(fh)
else:
    logging.warning("Yt logging disabled: '%s' doesn't exist" % Config.LOG_DIR)
