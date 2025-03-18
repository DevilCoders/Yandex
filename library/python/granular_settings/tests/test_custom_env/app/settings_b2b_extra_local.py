import os
import granular_settings


os.environ['YENV_TYPE'] = 'development'
os.environ['granular_environments'] = 'b2b,extra,local'

granular_settings.set(globals(), path='library/python/granular_settings/tests/test_custom_env/app/settings')
