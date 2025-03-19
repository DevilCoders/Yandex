import json
import sys
from uuid import uuid4
from pathlib import Path
from typing import Optional, Dict, Union


class DeploymentBuilder:
    """
    Provides methods for adding language models to use them in the subsequent deployment.
    """

    def __init__(self):
        """
        Creates empty DeploymentBuilder
        """
        self._lms = []

    @staticmethod
    def _create_desc(type: str, id: str, name: str):
        return {
            'type': type,
            'id': id,
            'name': name or id
        }

    def _validate_name(self, name: str):
        for lm in self._lms:
            if lm['name'] == name:
                raise ValueError(f'LM with name {name} already exists')

    def _validate_id(self, type: str, id: str):
        types = {'subword': 'Basic', 'light': 'Light'}
        for lm in self._lms:
            if lm['type'] == type and lm['id'] == id:
                raise ValueError(f'{types[type]} LM with id {id} already exists')

    def _add_language_model(self, lm_desc: Dict):
        self._validate_name(lm_desc['name'])
        self._validate_id(lm_desc['type'], lm_desc['id'])
        self._lms.append(lm_desc)

    def add_basic_language_model(self, id: str, name: Optional[str] = None) -> 'DeploymentBuilder':
        """
        Adds basic language model to deployment builder.
        :param id: language model id
        :param name: custom name of language model, `id` is used if `name` is not specified
        :return: `self`
        """
        lm_desc = self._create_desc('subword', id, name or f'basic_{id}')
        self._add_language_model(lm_desc)
        return self

    def add_light_language_model(self, id: str, name: Optional[str] = None) -> 'DeploymentBuilder':
        """
        Adds light language model to deployment builder.
        :param id: language model id
        :param name: custom name of language model, `id` is used if `name` is not specified
        :return: `self`
        """
        lm_desc = self._create_desc('light', id, name or f'light_{id}')
        self._add_language_model(lm_desc)
        return self

    def save(self, dump_path: Optional[Union[str, Path]]) -> None:
        """
        Saves deployment builder to use it in deployment.
        :param dump_path: path to save deployment builder
        """
        dump_path = Path(dump_path or str(uuid4()))
        with dump_path.open('w') as fp:
            json.dump(self._lms, fp, ensure_ascii=False, indent=2, sort_keys=True)
        sys.stdout.write(f'Deployment builder was successfully saved to {dump_path}\n')

    @staticmethod
    def load(dump_path: Union[str, Path]) -> 'DeploymentBuilder':
        """
        Loads dumped deployment builder from the specified file
        :param dump_path: path of the dumped deployment builder
        :return: deployment builder
        """
        dump_path = Path(dump_path)
        with Path(dump_path).open('r') as fp:
            dump = json.load(fp)

        deployment_builder = DeploymentBuilder()
        deployment_builder._lms = dump
        return deployment_builder
