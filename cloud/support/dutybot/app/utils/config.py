from yaml import (
    load,
    Loader
)
import logging

class Config:
    try:
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
            tg_debug = config['telegram']['debug']
            # Yandex services oAuth-tokens
            ya_staff = config['auth']['staff']
            ya_tracker = config['auth']['st']
            # Other
            useragent = config['other']['useragent']
            tracker_endpoint = config['other']['tracker_endpoint']
            staff_endpoint = config['other']['staff_endpoint']
            cloud_department = config['other']['department']
            resps_endpoint = config['other']['resps_endpoint']
            priority_types_startrek = {"1": "незначительный",
                                       "2": "нормальный",
                                       "3": "незначительный",
                                       "4": "критический",
                                       "5": "незначительный"}
    except FileNotFoundError:
        logging.warning("[config] Config file was not found during Config class init! Create config.yaml")