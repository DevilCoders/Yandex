import os
import logging

import pandas as pd

from adv.pcode.zfp.juggler_emulator.lib.emulator import prepare_data, JugglerEmulatorDataLoadError, JugglerAggregate as BaseJugglerAggregate


class JugglerAggregate(BaseJugglerAggregate):

    def load_unreach_checks(self, data_path):
        logging.error("Load unreach checks data")
        service_id = self.service.replace('_call', '')
        for check in self.unreach_checks:
            host, service = check.split(":")
            service = service.replace('#', service_id)
            file_path = os.path.join(data_path, f"{host}:{service}.tsv")
            if not os.path.exists(file_path):
                logging.error(f'{file_path} was not found')
                raise JugglerEmulatorDataLoadError()
            unreach_data = pd.read_csv(file_path, sep='\t')
            unreach_data = prepare_data(unreach_data)
            if self.unreach_checks_data is None:
                self.unreach_checks_data = unreach_data
            else:
                self.unreach_checks_data = pd.concat([self.unreach_checks_data, unreach_data], axis=1)
        if self.unreach_checks_data is not None:
            self.unreach_checks_data.columns = [f'unreach{i}' for i in range(len(self.unreach_checks))]
            logging.error(self.unreach_checks_data)

    def load_data(self, data_path):
        logging.error(str(self))
        service_id = self.service.replace('_call', '')
        for child in self.children:
            # Для посервисных алертов сервис сырого события должен быть равен сервису агрегата
            service = child['child'].get('service') or service_id
            service = service.replace('#', service_id)
            file_path =os.path.join(data_path, 'result', f'{child["child"]["host"]}', f'{child["child"]["host"]}__{service}.tsv')
            if not os.path.exists(file_path):
                logging.error(f'{file_path} was not found')
                raise JugglerEmulatorDataLoadError()
            children_data = pd.read_csv(file_path, sep='\t')
            children_data = prepare_data(children_data)
            # apply flap config for raw children
            children_data = self.flap_config.apply(children_data)
            # apply hold_crit for raw children after flap config
            children_data = self.hold_crit.apply(children_data)
            if self.data is None:
                self.data = children_data
            else:
                self.data = pd.concat([self.data, children_data], axis=1)
        self.data.columns = [f'raw{i}' for i in range(len(self.children))]
        logging.error(self.data)
