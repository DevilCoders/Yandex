from flask import Flask
import logging as log

app = Flask(__name__)

from app import routes
from app.config import *
app.config.from_object(Config)

if Config.LOGGING:
    log.basicConfig(filename=Config.LOGFILE,
                    level=Config.LOGLEVEL,
                    format='%(asctime)s %(levelname)s %(message)s')
