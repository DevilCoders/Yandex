import os


def _absolute_path(path):
    return os.path.join(os.path.abspath(os.path.dirname(__file__)), path)


MAIN_DIR = _absolute_path('.')


MAIN_DB_DIR = os.path.join(MAIN_DIR, "db")

PERFORMANCE_DEBUG = False

# TODO: add in all tempfile calls
TEMPFILE_PREFIX = 'gcfg'
GENERATED_DIR = 'generated'
WEB_GENERATED_DIR = 'w-generated'

TAG_PATTERN = '^stable-(\d+)-r(\d+)$'

BRANCH_VERSION = "158"

ARCADIA_RELATIVE_REGEX = "(?:https|svn\+ssh):\/\/(?:.*@?)arcadia(?:\.yandex\.ru)?/arc(.*)"

GENCFG_TRUNK_DATA_PATH = "svn+ssh://arcadia.yandex.ru/arc/trunk/data/gencfg_db"
GENCFG_TRUNK_SRC_PATH = "svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/gencfg"
GENCFG_BRANCH_PREFIX = "svn+ssh://arcadia.yandex.ru/arc/branches/gencfg"
GENCFG_TAG_PREFIX = "svn+ssh://arcadia.yandex.ru/arc/tags/gencfg"

REPO_MAPPING = {
    "full": "",
    "db": "/db",
    "balancer": "/custom_generators/balancer_gencfg",
}

EINE_PROP_LAN_URL = 'http://eine.yandex-team.ru/computer/get_prop/lan.cli'

SERVICE_EXIT_CODE_OTHER_ERR = 64
SERVICE_EXIT_CODE_SOCKET_ERR = 65

VENV_REL_PATH = os.path.join('venv/venv')
VENV_DIR = os.path.join(MAIN_DIR, VENV_REL_PATH)

GENCFG_GROUP_CARD_WIKI_URL = "/jandekspoisk/sepe/gencfg/groupcardauto/"

SCHEME_LEAF_DOC_FILE = "doc.md"


def get_default_oauth():
    if 'GENCFG_DEFAULT_OAUTH' not in os.environ:
        raise Exception('Variable <GENCFG_DEFAULT_OAUTH> with default oauth token is not found in environ')

    return os.environ['GENCFG_DEFAULT_OAUTH']
