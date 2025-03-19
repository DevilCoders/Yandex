from flask import Flask
import logging as log
import os

app = Flask(__name__,
            template_folder=os.getcwd()+'/templates',
            static_folder=os.getcwd()+'/static',
            )

from app.config import Config
from app import routes

if Config.LOGGING:
    log.basicConfig(filename=Config.LOGFILE,
                    level=Config.LOGLEVEL,
                    format='%(asctime)s %(levelname)s %(message)s')

# F401 routes will be used
print(routes)
