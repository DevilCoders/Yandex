import granular_settings
import os

VARNAME = 'MYPROJECT_SETTINGS'
os.environ[VARNAME] = 'library/python/granular_settings/tests/test_path/etc/myapp'

settings = granular_settings.from_envvar(variable_name='MYPROJECT_SETTINGS')
