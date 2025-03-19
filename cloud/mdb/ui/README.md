## Running local dev server

#### Environment
```bash
sudo apt-get install python3-dev python3-pip python3-venv
python3 -m venv env
source env/bin/activate
pip install -r requirements.txt
python src/manage.py migrate
```

#### vim src/config.ini
```
[main]
BASE_HOST = 127.0.0.1:8000
DEBUG = True
SECRET_KEY = <any string>
INSTALLATION = porto-test

[auth]

[deploydb]
HOST = deploy-db-test01i.db.yandex.net,deploy-db-test01h.db.yandex.net,deploy-db-test01k.db.yandex.net
PASSWORD = < mdb_ui password >

[metadb]
HOST = meta-test01i.db.yandex.net,meta-test01k.db.yandex.net,meta-test01h.db.yandex.net
PASSWORD = < mdb_ui password >

[katandb]
HOST = katan-db-test01h.db.yandex.net,katan-db-test01i.db.yandex.net,katan-db-test01k.db.yandex.net
PASSWORD = < mdb_ui password >


[dbmdb]
HOST = ddbm-test01h.db.yandex.net,dbm-test01i.db.yandex.net,dbm-test01k.db.yandex.net
PASSWORD = < mdb_ui password >

[cmsdb]
HOST = cms-db-test01i.db.yandex.net,cms-db-test01h.db.yandex.net,cms-db-test01k.db.yandex.net
PASSWORD = < mdb_ui password >

```

#### Run

```bash
cd src; python manage.py runserver
```

mdb_ui password here https://yav.yandex-team.ru/secret/sec-01eg075sms3f45q75495ywean6/explore/versions

deb here https://jenkins.db.yandex-team.ru/view/DEB/job/mdb-ui-deb/
