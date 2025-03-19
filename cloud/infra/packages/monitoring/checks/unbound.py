#!/usr/bin/env python3
from yc_monitoring import check_systemd_service_status

SERVICE_NAME = "unbound"
SERVICE_UPTIME_TRESHOLD_SEC = 60


def main():
    print(check_systemd_service_status(SERVICE_NAME, SERVICE_UPTIME_TRESHOLD_SEC))

if __name__ == '__main__':
    main()
