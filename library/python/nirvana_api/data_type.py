class DataType(object):
    TSV = 'tsv'
    JSON = 'json'
    XML = 'xml'
    HTML = 'html'
    Image = 'image'
    Text = 'text'
    Binary = 'binary'
    Executable = 'executable'
    FMLPool = 'fmlPool'
    FMLPRS = 'fmlPRS'
    FMLFormula = 'fmlFormula'
    FMLFormulaSerpPrefs = 'fmlFormulaSerpPrefs'
    FMLSerpComparison = 'fmlSerpComparison'
    MRTable = 'mrTable'
    MRDirectory = 'mrDirectory'
    MRFile = 'mrFile'
    hiveTable = 'hiveTable'


class FMLPool(object):
    def __init__(self, fml_id):
        self.id = fml_id


class FMLPRS(object):
    def __init__(self, fml_id):
        self.id = fml_id


class FMLFormula(object):
    def __init__(self, fml_id, fml_type):
        self.id = fml_id
        self.type = fml_type


class FMLSerpComparison(object):
    def __init__(self, fml_id, results_url, graph_url):
        self.id = fml_id
        self.results_url = results_url
        self.graph_url = graph_url


class MRTable(object):
    def __init__(self, cluster, table, host=None, port=None):
        self.cluster = cluster
        self.host = host
        self.port = port
        self.table = table
