import os, codecs

SECRET_KEY = codecs.encode(os.urandom(32), 'hex_codec').decode('utf-8')

DATABASES = {
    'default': {
        'ENGINE': 'django.db.backends.sqlite3',
        'NAME': 'test.sqlite',
     }
}
