import logging
from .service.jslogger import JSLogger

logging.setLoggerClass(JSLogger)
logging.root = JSLogger('root', request_id="None", service_id="Unknown", cfg_version="None")
