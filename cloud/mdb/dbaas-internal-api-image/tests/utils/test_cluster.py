"""
Test for cluster utils
"""
import pytest

from dbaas_internal_api.utils.cluster.update import __name__ as PACKAGE
from dbaas_internal_api.utils.cluster.update import update_cluster_description, update_cluster_labels
from dbaas_internal_api.utils.types import ClusterInfo

# pylint: disable=invalid-name

COLOR_RED = {
    'color': 'red',
}
COLOR_BLUE = {
    'color': 'blue',
}
COLOR_RED_AND_VERSION_NEW = {
    'color': 'red',
    'version': 'new',
}


class Test_update_cluster_labels:
    """
    Test for update_cluster_labels
    """

    test_cid = 'test_cid'

    def _mock(self, mocker):
        set_labels_mock = mocker.patch(PACKAGE + '.metadb.set_labels_on_cluster')
        return set_labels_mock

    def _cluster(self, labels):
        cd = dict.fromkeys(ClusterInfo._fields)
        cd['labels'] = labels
        cd['cid'] = self.test_cid
        return cd

    def test_when_new_labels_not_defined(self, mocker):
        """
        Should not change them, when labels is None
        """
        set_labels_mock = self._mock(mocker)
        assert (
            update_cluster_labels(
                cluster=self._cluster(
                    {
                        'color': 'red',
                    }
                ),
                labels=None,
            )
            is False
        )
        set_labels_mock.assert_not_called()

    def test_when_new_labels_are_same(self, mocker):
        """
        Should not change them, when labels is None
        """
        set_labels_mock = self._mock(mocker)
        assert (
            update_cluster_labels(
                cluster=self._cluster(
                    {
                        'color': 'red',
                        'version': 'latest',
                    }
                ),
                labels={
                    'version': 'latest',
                    'color': 'red',
                },
            )
            is False
        )
        set_labels_mock.assert_not_called()

    @pytest.mark.parametrize(
        ['cluster_labels', 'new_labels'],
        [
            ({}, COLOR_RED),
            (COLOR_RED, {}),
            (COLOR_RED, COLOR_BLUE),
            (COLOR_RED_AND_VERSION_NEW, COLOR_RED),
            (COLOR_RED, COLOR_RED_AND_VERSION_NEW),
        ],
    )
    def test_when_new_labels_are_diffrent(self, mocker, cluster_labels, new_labels):
        """
        Should change labels and return True
        """
        set_labels_mock = self._mock(mocker)
        assert update_cluster_labels(cluster=self._cluster(cluster_labels), labels=new_labels) is True
        set_labels_mock.assert_called_once_with(
            cid=self.test_cid,
            labels=new_labels,
        )

    def test_raise_when_got_cluster_dict_without_labels(self):
        """
        Got unexpected dict
        """
        cd = self._cluster({})
        cd.pop('labels')
        with pytest.raises(RuntimeError):
            update_cluster_labels(cluster=cd, labels={})


class Test_update_cluster_description:
    test_cid = 'test_cid'

    def _mock(self, mocker):
        update_mock = mocker.patch(PACKAGE + '.metadb.update_cluster_description')
        return update_mock

    def _cluster(self, description):
        cd = dict.fromkeys(ClusterInfo._fields)
        cd['cid'] = self.test_cid
        cd['description'] = description
        return cd

    @pytest.mark.parametrize(
        ['cluster_description', 'new_description'],
        [
            (None, None),
            ('xxx', None),
            ('xxx', 'xxx'),
        ],
    )
    def test_dont_update(self, mocker, cluster_description, new_description):
        """
        Return False and don't call metadb when description unchanged
        """
        update_mock = self._mock(mocker)
        cd = self._cluster(cluster_description)
        assert update_cluster_description(cluster=cd, description=new_description) is False
        update_mock.assert_not_called()

    @pytest.mark.parametrize(
        ['cluster_description', 'new_description'],
        [
            (None, 'xxx'),
            ('xxx', 'yyy'),
        ],
    )
    def test_update(self, mocker, cluster_description, new_description):
        """
        Return True and call metadb when description changed
        """
        update_mock = self._mock(mocker)
        cd = self._cluster(cluster_description)
        assert update_cluster_description(cluster=cd, description=new_description) is True
        update_mock.assert_called_once_with(
            cid=self.test_cid,
            description=new_description,
        )

    def test_raise_when_got_cluster_dict_without_description(self):
        """
        Got unexpected dict
        """
        cd = self._cluster({})
        cd.pop('description')
        with pytest.raises(RuntimeError):
            update_cluster_description(cluster=cd, description='xxx')
