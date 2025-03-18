import click
import json
import time
import math
from infra.yasm.yasmapi import GolovanRequest


SECONDS_IN_HOUR = 3600
SECONDS_IN_5MIN = 300


def golovan_request(period, ts_start, ts_end, signals):
    return list(GolovanRequest("ASEARCH", period, ts_start, ts_end, signals))


@click.group()
def main():
    pass


@main.command('fetch_stat')
@click.option('--service-config-file', type=str, required=True)
@click.option('--max-save-rps', type=int, default=10)
def fetch_stat(service_config_file, max_save_rps):
    with open(service_config_file) as f:
        data = json.load(f)

    now_ts = int(time.time()) - 2*SECONDS_IN_HOUR

    new_data = data
    for i, service_data in enumerate(data):
        service = service_data['service']
        fraction = float(service_data['random_factors_fraction'])

        result = golovan_request(SECONDS_IN_HOUR, now_ts - SECONDS_IN_HOUR, now_ts, [f"itype=antirobot;service_type={service}:div(unistat_daemon-requests_passed_to_service_deee, {SECONDS_IN_HOUR})"])
        rps = float(list(result[0][1].values())[0])
        if rps != 0:
            new_fraction = round(min(float(max_save_rps) / rps, 1.0), 5)
            if math.isclose(fraction, 0):
                new_data[i]['random_factors_fraction'] = new_fraction

            print(service, fraction, new_fraction, rps)

        with open(service_config_file, 'w') as f:
            print(json.dumps(new_data, indent=4, sort_keys=True), file=f)

if __name__ == "__main__":
    main()
