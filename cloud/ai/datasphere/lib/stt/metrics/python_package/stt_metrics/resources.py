import json
from typing import List, Union, Optional
from pathlib import Path
from collections import defaultdict


class ClusterReferences:
    """
    A set of clusters of words, which should be treated equally when evaluating metrics.
    """
    def __init__(self):
        self._clusters = {}
        self._alias_to_centers = defaultdict(list)

    def add_cluster(self, center: str, aliases: List[str]) -> 'ClusterReferences':
        """
        Adds cluster of words, which should be treated equally.
        Each cluster is described by its center and a set of center's aliases.

        This is an in-place function.

        :param center: str
            Center of the cluster.
        :param aliases: List[str]
            List of aliases.

        :return: self
        """
        self._clusters[center] = aliases.copy()
        center = tuple(center.split())
        for alias in aliases:
            alias = tuple(alias.split())
            self._alias_to_centers[alias].append(center)
        return self

    def get(self, value: str, default_value: str):
        """
        Try to get a cluster center from cluster references. If it doesn't exists, returns specified default value.

        :param value: str
            Some value from an existing cluster.
        :param default_value: str
            Default value for the case when no cluster was found.

        :return: the center of a cluster where the value is located (or default_value if no cluster was found)
        """
        return self._alias_to_centers.get(value, default_value)

    def save(self, path: Union[str, Path]) -> None:
        """
        Stores cluster references to the specified file.

        :param path: str or Path
            Path to store cluster references.
        """
        Path(path).write_text(json.dumps(self._clusters, indent=2))

    @staticmethod
    def load(path: Union[str, Path]) -> 'ClusterReferences':
        """
        Loads cluster references from the specified file.

        :param path: str or Path
            Path to load cluster references from.
        """
        cr = ClusterReferences()
        clusters = json.loads(Path(path).read_text())
        for center, aliases in clusters.items():
            cr.add_cluster(center, aliases)
        return cr

    def __repr__(self):
        repr = ''
        for center, aliases in self._clusters.items():
            repr += '{} --> {}'.format(center, aliases) + '\n'
        return repr
