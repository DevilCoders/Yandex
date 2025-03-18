# -*- coding: utf-8 -*-

import logging

root_logger = logging.getLogger()
root_logger.setLevel(logging.DEBUG)

from ci.api_client.py.ci_yp_service_discovery import get_all_endpoints


def main():
    endpoints = get_all_endpoints('test', cluster_names=['man', ])
    logging.info("Endpoints: %s", endpoints)

if __name__ == '__main__':
    main()
