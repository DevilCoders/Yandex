# YC Support Bot
  
**Yandex.Cloud Telegram Bot for Support team**  
  
_The ya.make build doesn't work because contib's python-telegram-bot in arcadia was updated 10 months ago..._  
  
## Get started
  
### Create database & tables
  
```bash
mysql -uroot -p
```
  
```sql
CREATE DATABASE yc_support_bot DEFAULT CHARACTER SET utf8;
DEFAULT COLLATE utf8_general_ci;
GRANT ALL PRIVILEGES ON yc_support_bot.* TO 'my-user-name' IDENTIFIED BY 'my-strong-password';
FLUSH PRIVILEGES;
\q
```
  
**Install tables to database from `sql/install.sql`**

```bash
mysql yc_support_bot --user my-user-name -p < sql/install.sql
```
  

### Get credentials
- Get Telegram API Token for Bot from [BotFather](https://t.me/botfather)
- Get Yandex internal token [here](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=a7597fe0fbbf4cd896e64f795d532ad2)
- Create MySQL Database with name `yc_support_bot`

### Config Setup
  
* Create config file: `mkdir -p ~/.support-bot && vim ~/.support-bot/config.yaml`
  
```yaml
telegram:
  token: 123456:AAAbbbbeeee....
  endpoint: 

startrek:
  token: AQAD-qqqqqqq....
  endpoint: 

staff:
  token: AQAD-qqqqqq....
  endpoint: 

resps:
  token: 
  endpoint: 

database:
  host: database.endpoint.ru
  port: 3306
  user: my-user-name
  passwd: strong-password
  db_name: yc_support_bot
  ca_path: /path/to/root.crt

logs:
  path: /home/my-username/
  level: info
  max_size:
  backup_count:
  disable_telegram_debug: true
  disable_startrek_debug: true
```
  
**The maximum file size must be specified in bytes**  
**Log levels:** debug, info, warning, error.
  

### Prepare environment
  
* Create and activate virtual environment:  
```bash
python3 -m venv your-venv-name
source your-venv-name/bin/activate
```
  
* Install internal requirements:
```bash
pip install -i https://pypi.yandex-team.ru/simple/ startrek_client
```
  
* Install external requirements:
```bash
pip install -r requirements.txt
```
  

### Usage
  
```
Yandex.Cloud Support Telegram Bot

python3 main.py            run bot

Optional arguments:
  -h, --help        show this help message and exit
  --version         show program's version number and exit
  -l, --write-logs  write logs to /var/log/ or path from config.
```

## How to add new worker

* Create module in workers dir like a: `workers/myworker.py`
* Create class MyWorker in module, example:
```python
#!/usr/bin/env python3
"""This module contains YOUR_WORKER_NAME class."""

import logging
import threading  # for example

logger = logging.getLogger(__name__)


class YourWorker:  # change this
    """This is example."""

    def __init__(self):
        logger.info(f'Loading worker {type(self).__name__}...')  # required
        self.threads = threading.activeCount()

    def secondary_func(self):
        message = f'Active threads: {self.threads}'
        return message

    def another_secondary_func(self, *args, **kwargs):
        ...

    def run(self, context: object):  # required, entry point for starting worker
        """Main worker func."""
        print('My worker is the best on the planet!')
        print(self.another_secondary_func())
```

* Import your worker to `workers/__init__.py`, example:
```python
#!/usr/bin/env python3
"""This module contains all workers for TelegramBot."""

import datetime

from .notifier import SupportNotifier
from .watchdog import SecurityWatchdog
from .myworker import YourWorkerName  # new

# REPEATING WORKERS
# WorkerClass: (loop interval, run worker N seconds after bot start)
repeating_workers = {
    SupportNotifier: (60, 3),
    YourWorkerName: (600, 5),  # new. interval and first start must be int (seconds)
}

# DAILY WORKERS
# WorkerClass: datetime.time(hour, minutes, seconds)
daily_workers = {
    SecurityWatchdog: datetime.time(21, 00, 00)
}

```
* Make sure everything works correctly...
* Profit!
