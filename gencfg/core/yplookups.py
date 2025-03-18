import itertools
import logging
import os

import core.card.node as node
from gaux.aux_decorators import memoize


class YpLookup(object):
    """Similar to intlookup, but over YP and Service Discovery"""

    def __init__(self, name, cluster, base, intl1, intl2):
        self.name = name
        self.cluster = cluster
        self.base = base
        self.intl1 = intl1
        self.intl2 = intl2

    def base_shard_prefix(self, shard_number):
        return self.base.endpoint_set_format.format(cluster=self.cluster, pod_set_id=self.base.pod_set_id,
                                                    shard_number=shard_number,
                                                    shard_id=self.base.tier.get_shard_id(shard_number))

    def __str__(self):
        return '{}@{}'.format(self.cluster, self.name)


class SearchSource(object):
    def __init__(self, layer, n, subsources):
        self._layer = layer
        self._n = n
        self._subsources = subsources

    @property
    def layer(self):
        return self._layer

    @property
    def index(self):
        return self._n

    @property
    def tier(self):
        return self._layer.tier

    @property
    def subsources(self):
        return self._subsources

    @property
    def primus_list(self):
        if self._layer.tier:
            return [self._layer.tier.get_shard_id(self._n)]
        elif self._subsources:
            return list(itertools.chain.from_iterable(x.primus_list for x in self._subsources))
        else:
            raise RuntimeError('Inapplicable')

    @property
    def sd_expression(self):
        return self._layer.endpoint_set_format.format(cluster=self._layer.cluster,
                                                      pod_set_id=self._layer.pod_set_id,
                                                      shard_number=self._n,
                                                      shard_id=self.shard_id)

    @property
    def config_filename(self):
        return '{}.{}.cfg'.format(self._layer.pod_set_id, self.shard_id)

    @property
    def shard_id(self):
        if self._layer.tier:
            return self._layer.tier.get_shard_id(self._n)
        return self._layer.shard_id_format.format(shard_number=self._n)

    @property
    def service_name(self):
        return self._layer.pod_set_id

    def name(self):
        raise NotImplementedError()

    def __str__(self):
        return 'SearchSource({})'.format(self.sd_expression)


class Layer(object):
    def __init__(self, card_node, base, cluster):
        self._card_node = card_node
        self._next_layer = base
        self._cluster = cluster

    @property
    def cluster(self):
        return self._cluster

    @property
    def endpoint_set_format(self):
        return self._card_node.endpoint_set_format

    @property
    def shard_id_format(self):
        return self._card_node.shard_id_format

    @property
    def group_size(self):
        return self._card_node.group_size

    @property
    def pod_set_id(self):
        return self._card_node.pod_set_id

    @property
    @memoize
    def sources(self):
        if not self._card_node:
            return []

        group_size = self._next_layer.group_size
        sources = []
        for i, n in enumerate(range(0, len(self._next_layer.sources), group_size)):
            subsources = self._next_layer.sources[n: n + group_size]
            sources.append(SearchSource(self, i, subsources))
        return sources

    @property
    def tier(self):
        return self._card_node.get('tier')


class Base(Layer):
    @property
    @memoize
    def sources(self):
        return [SearchSource(self, n, None) for n in range(self.tier.get_shards_count())]


class YpLookups(object):
    DEFAULT_SERVICE_DISCOVERY_EXPRESSION = 'sd://{cluster}@{pod_set_id}-{shard_id}/yandsearch'
    DEFAULT_SHARD_ID_FORMAT = '{shard_number}'

    def __init__(self, db):
        self._db = db
        self._yplookups = {}
        self._modified = False

        if self._db.version < '2.2.57':
            return

        self._scheme = node.Scheme(os.path.join(self._db.SCHEMES_DIR, 'yplookup.yaml'), self._db.version)
        self._yplookups = self._load_yplookups()

    def get_yplookup_by_name(self, name):
        """:returns Dict[str, YpLookup]"""
        # GENCFG-4401 (support multiple endpoints in yplookup)
        names = name.split('+')
        return [self._yplookups[name] for name in names]

    def mark_as_modified(self):
        self._modified = True

    def update(self, smart=False):
        pass

    def fast_check(self, timeout):
        pass

    def _load_yplookups(self):
        yplookups = {}
        for filename in os.listdir(self._db.YPLOOKUP_DIR):  # todo: skip .SAS_WEB_PLATINUM_HAMSTER.swp
            absolute_filename = os.path.join(self._db.YPLOOKUP_DIR, filename)
            if not os.path.isfile(absolute_filename):
                logging.debug('skipping %s', absolute_filename)
                continue
            content = node.CardNode.load_json(absolute_filename, self._scheme)
            # noinspection PyUnresolvedReferences
            base = Base(self._load_layer(content.base), None, content.cluster)
            intl1 = Layer(self._load_layer(content.intl1), base, content.cluster)
            intl2 = Layer(self._load_layer(content.intl2), intl1, content.cluster)
            yplookup = YpLookup(content.name, content.cluster,
                                base=base, intl1=intl1, intl2=intl2)
            yplookups[yplookup.name] = yplookup
            # logging.debug('load %s from %s', yplookup.name, absolute_filename)
        return yplookups

    def _load_layer(self, card_node):
        if hasattr(card_node, 'tier'):
            card_node.tier = self._db.tiers.get_tier(card_node.tier)
        if not card_node.endpoint_set_format:
            card_node.endpoint_set_format = self.DEFAULT_SERVICE_DISCOVERY_EXPRESSION
        if not card_node.shard_id_format:
            card_node.shard_id_format = self.DEFAULT_SHARD_ID_FORMAT
        return card_node if card_node.replicas > 0 else None
