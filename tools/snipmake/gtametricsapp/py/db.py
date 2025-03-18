import MySQLdb

class DBWorker:
    def __init__(self, config):
        self.db = None
        self.cfg = config

    def connect(self):
        self.db = MySQLdb.connect(host=self.cfg.dbHost, port=self.cfg.dbPort, db=self.cfg.dbName, user=self.cfg.dbUser, passwd=self.cfg.dbPasswd, connect_timeout=5)

    def getCursor(self):
        try:
            return self.db.cursor()
        except:
            self.connect()
            return self.db.cursor()

    def __del__(self):
        if self.db:
            self.db.commit()
            self.db.close()
