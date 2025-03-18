# coding: utf-8

# ID проекта в Танкере
TANKER_ID = None

# Кейсет проекта
TANKER_KEYSET = None

# Мапинг приложение - кейсет
TANKER_APP_KEYSETS = {}

# Обрабатываемые языки
TANKER_LANGUAGE = ('ru', 'uk', 'kz', 'en', 'be')

# Дефолтный режим загрузки переводов
TANKER_MODE = 'replace'  # [replace, update, merge]

# Флаг использования тестового Танкера
TANKER_USE_TESTING = False

# Флаг используется, когда ключ не является осмысленным переводом на каком-то
# языке. Если флага нет, то ключ используется как перевод для дефолтного языка (Русский)
TANKER_KEY_NOT_LANGUAGE = False

# Токен для работы с API
# Если работает без него, значит он вам пока не нужен,
# иначе получите токен по адресу:
# https://nda.ya.ru/3SjQY7
# и поменяйте эту переменную на:
# TANKER_TOKEN = 'OAuth <token>'
TANKER_TOKEN = None

TANKER_REVISION = 'master'
