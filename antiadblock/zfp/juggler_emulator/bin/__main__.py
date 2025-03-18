import os
import argparse
import logging

import yaml

from antiadblock.tasks.tools.configs_api import get_configs
from antiadblock.tasks.tools.const import VALUABLE_SERVICES
from antiadblock.zfp.juggler_emulator.lib.emulator import JugglerAggregate
from adv.pcode.zfp.juggler_emulator.lib.emulator import JugglerEmulatorDataLoadError

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--result_dir', required=True)
    parser.add_argument('--data_dir', required=True)
    parser.add_argument('--aggregate_config', required=True)

    args = parser.parse_args()

    tvm_id = int(os.getenv('TVM_ID', '2002631'))
    configsapi_tvm_id = int(os.getenv('CONFIGSAPI_TVM_ID', '2000629'))
    tvm_secret = os.getenv('TVM_SECRET')
    configs_api_host = os.getenv("CONFIGS_API_HOST", 'api.aabadmin.yandex.ru')

    service_ids = get_configs(tvm_id, tvm_secret, configsapi_tvm_id, configs_api_host, monitorings_enabled=True).keys()
    with open(args.aggregate_config) as f:
        config = yaml.safe_load(f)

    for name, params in config.items():
        aggregates = []
        juggler_host, juggler_service = name.split(':')
        if juggler_service == '*':
            aggregates += [JugglerAggregate(juggler_host, service_id,
                                            params.get('flaps_config', {}), params['children'],
                                            params['aggregator_kwargs'], params['aggregator']) for service_id in service_ids]
        elif juggler_service == '*_call':
            aggregates += [JugglerAggregate(juggler_host, f'{service_id}_call',
                                            params.get('flaps_config'), params['children'],
                                            params['aggregator_kwargs'], params['aggregator']) for service_id in VALUABLE_SERVICES]

        else:
            aggregates.append(JugglerAggregate(juggler_host, juggler_service,
                                               params.get('flaps_config', {}), params['children'],
                                               params['aggregator_kwargs'], params['aggregator']))
        for aggregate in aggregates:
            try:
                aggregate.calculate(args.data_dir)
            except JugglerEmulatorDataLoadError:
                logging.error(f'Data absent for {aggregate}')
            aggregate.save_result(args.result_dir)
            aggregate.save_result(args.data_dir, write_header=True)
