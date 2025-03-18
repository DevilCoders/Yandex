import sys
import requests
import logging

if __name__ == "__main__":
    try:
        r = requests.get("http://localhost:5000")
        if r.status_code != 200:
            logging.warning("Unexpected status response: %d" % r.status_code)
            sys.exit(1)
    except ConnectionRefusedError:
        logging.warning("Connection refused")
        sys.exit(1)
