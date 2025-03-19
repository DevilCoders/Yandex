import typing as tp
import pandas as pd
from nile.api.v1.record import Record
from nile.api.v1 import clusters, Job
import logging

logger = logging.getLogger(__name__)


class NileDataAdapter:

    def __init__(self,  yt_token: tp.Optional[str] = None, yql_token: tp.Optional[str] = None,
                 yt_layers_path: tp.Union[tp.List[str], str] = 'default', cluster: str = 'hahn',
                 pool: str = 'cloud_analytics_pool', checkpoints_path: str = '//home/cloud_analytics/tmp', backend: str = 'yt') -> None:

        layers: tp.Union[tp.List[str], str, None] = None
        if yt_layers_path == 'default':
            layers = [
                '//porto_layers/base/bionic/porto_layer_search_ubuntu_bionic_app-2020-04-22-17.52.26.tar.gz',
                '//home/cloud_analytics/porto/py37.tar.xz'
            ]
        else:
            layers = yt_layers_path

        yt_spec_defaults = dict(
            mapper=dict(layer_paths=layers),
            reducer=dict(layer_paths=layers),
        )
        conf = dict(token=yt_token, proxy=cluster, pool=pool)
        _cluster = None
        if backend == 'yt':
            _cluster = clusters.YT(**conf)
        elif backend == 'yql':
            _cluster = clusters.YQLProduction(**conf, yql_token=yql_token)
        else:
            raise KeyError(f'`backend` must be one of "yt" or "yql". It has value {backend}')
        self._cluster = _cluster.env(yt_spec_defaults=yt_spec_defaults, templates=dict(checkpoints_root=checkpoints_path))

    def job(self, name: str) -> Job:
        return self._cluster.job(name)

    @property
    def env(self) -> clusters.env:
        return self._cluster.env

    @property
    def driver(self) -> clusters.driver:
        return self._cluster.driver

    def write(self, path: str, df: pd.DataFrame) -> tp.Any:
        return self._cluster.write(path, df)

    def dummy_write(self) -> None:
        self._cluster.write('//tmp/dummy', [Record(n=1)], schema={'n': int})


class YTNileAdapter(NileDataAdapter):

    def __init__(self, yt_layers_path: str = 'default', cluster: str = 'hahn', pool: str = 'cloud_analytics_pool',
                 checkpoints_path: str = '//home/cloud_analytics/tmp') -> None:

        super().__init__(yt_layers_path=yt_layers_path, cluster=cluster,
                         pool=pool, checkpoints_path=checkpoints_path, backend='yt')


class YQLNileAdapter(NileDataAdapter):

    def __init__(self, yt_layers_path: str = 'default', cluster: str = 'hahn', pool: str = 'cloud_analytics_pool',
                 checkpoints_path: str = '//home/cloud_analytics/tmp') -> None:

        super().__init__(yt_layers_path=yt_layers_path, cluster=cluster,
                         pool=pool, checkpoints_path=checkpoints_path, backend='yql')
