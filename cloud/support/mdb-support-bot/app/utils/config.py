from yaml import (
    load,
    Loader
)
import logging

class Config:
    with open("config.yaml", "r") as config:
        config = load(config, Loader=Loader)
        # Database
        db_name = config['database']['name']
        db_user = config['database']['user']
        db_pass = config['database']['pass']
        db_host = config['database']['host']
        db_port = config['database']['port']
        # Telegram
        tg_prod = config['telegram']['prod']
        # Yandex services other
        ya_staff = config['auth']['staff']
        useragent = config['other']['useragent']
        staff_endpoint = config['other']['staff_endpoint']
        cloud_department = config['other']['department']