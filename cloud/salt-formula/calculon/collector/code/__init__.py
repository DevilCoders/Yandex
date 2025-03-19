from flask import Flask
app = Flask(__name__)
app.config.from_envvar('calculon_config', silent=True)
from collector import app
