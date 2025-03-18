
class ETodayTableNotExists(Exception):
    def __init__(self, table = ""):
        self.tableName = table

    def __str__(self):
        return "Precalced table '%s' for current day does not exist" % self.tableName

class EOptTableNotExists(Exception):
    def __init__(self, table = ""):
        self.tableName = table

    def __str__(self):
        return "Optimized table '%s' does not exist" % self.tableName

class ESlowTableNotExists(Exception):
    def __init__(self, table = ""):
        self.tableName = table

    def __str__(self):
        return "Regular table '%s' does not exist" % self.tableName

class ESlowFetchRequired(Exception):
    pass

class EAlreadyPending(Exception):
    pass

class ELogicError(Exception):
    pass

class EMapreduceError(Exception):
    pass
