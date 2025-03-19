# Check scripts

Writing scripts in python one can use useful utilities:

Manage status for monrun
```
from monrun_output import Status, die, try_or_die

def all_my_executable_code_without_arguments():
    die(Status.OK, 'a dummy check')

# this will handle all errors and guaranteed to output something sensible to monrun
try_or_die(all_my_executable_code_without_arguments)

```

You can use them considering your scripts are in standart place `/usr/local/yandex/monitoring`.
This is granted by `./common/init.sls`
