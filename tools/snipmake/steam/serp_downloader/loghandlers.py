#!/usr/bin/python
# -*- coding: utf-8 -*-


import sys;
import logging;


class StreamLogHandler(logging.StreamHandler):

    def __init__(self, stream=sys.stderr):
        logging.StreamHandler.__init__(self, stream);
        self.setFormatter(logging.Formatter(fmt="%(asctime)s: %(message)s",
                                            datefmt="%H:%M:%S %d.%m.%Y"));
        self.setLevel(logging.INFO);


class FileLogHandler(logging.FileHandler):

    def __init__(self, filename, mode="a"):
        logging.FileHandler.__init__(self, filename, mode);
        self.setFormatter(logging.Formatter(fmt="%(asctime)s: %(message)s",
                                            datefmt="%H:%M:%S %d.%m.%Y"));
        self.setLevel(logging.INFO);


class SteamLogger(logging.Logger):

    logger = None;
    name = "steam";
    level = logging.INFO;
    handlers =[];

    def __init__(self, name="steam"):
        if SteamLogger.logger == None:
            logging.Logger.__init__(self, "steam");
            self.addHandler(StreamLogHandler());
            self.addHandler(FileLogHandler(LOG_FILE));
            SteamLogger.handlers = self.handlers;
            self.setLevel(logging.INFO);
            SteamLogger.logger = self;

    @staticmethod
    def prepare_msg(msg, params):
        if not params.get("type"):
            params["type"] = "";
        return "".join(("%(type)s, ", msg)) % params;

    @staticmethod
    def info(msg, *args, **kwargs):
        logging.Logger.info(SteamLogger.logger,
                            SteamLogger.prepare_msg(msg, kwargs));

    @staticmethod
    def warning(msg, *args, **kwargs):
        logging.Logger.warning(SteamLogger.logger,
                               SteamLogger.prepare_msg(msg, kwargs));

    @staticmethod
    def error(msg, *args, **kwargs):
        logging.Logger.error(SteamLogger.logger,
                             SteamLogger.prepare_msg(msg, kwargs));


if "loghandlers" in __name__:
    try:
        from core.settings import LOG_FILE
    except ImportError:
        LOG_FILE = "steam.log"
    SteamLogger();

