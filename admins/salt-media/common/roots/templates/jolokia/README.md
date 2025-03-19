Подключение агента !!! Без агента ничего не заработает
=======================================================
https://jolokia.org/agent/jvm.html


Чтобы эта штука заработала
=========================

1) подключить темплейт для нужного кластера.
2) добавить pillar для этого же кластера
    ```
    jolokia:
        conf: salt://<cluster>/myconfig.yaml
    ```
    Пример конфига смотреть тут ./conf.yaml

3) написать этот самый конфиг и положить туда, куда смотрит pillar.
4) раскатить стейт.


Для того, чтобы понять какие метрики можно выдергивать из java.
Можно воспользоваться вот таким скриптом
```(python)
#!/usr/bin/env python
# -*- coding: utf-8 -*-
import json
from pyjolokia import Jolokia

j4p = Jolokia('http://localhost:8778/jolokia/')

resp = j4p.request(type = 'list', path='')
print(json.dumps(resp, indent=2))

resp = j4p.request(type = 'search', mbean='java.lang:*')
print(json.dumps(resp, indent=2))
```

Подробности тут
===============
https://st.yandex-team.ru/MDADM-3290

https://jolokia.org
https://github.com/cwood/pyjolokia
