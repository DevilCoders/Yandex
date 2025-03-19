package imp

import (
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func UnmarshalClusters(clusters []katandb.Cluster) (map[string]tags.ClusterTags, error) {
	ret := make(map[string]tags.ClusterTags, len(clusters))

	for _, c := range clusters {
		t, err := tags.UnmarshalClusterTags(c.Tags)
		if err != nil {
			return nil, xerrors.Errorf("unable unmarshal %q tags: %w", c.ID, err)
		}
		ret[c.ID] = t
	}
	return ret, nil
}

func UnmarshalHosts(hosts []katandb.Host) (map[string]tags.HostTags, error) {
	ret := make(map[string]tags.HostTags, len(hosts))
	for _, h := range hosts {
		t, err := tags.UnmarshalHostTags(h.Tags)
		if err != nil {
			return nil, xerrors.Errorf("unable unmarshal %q tags: %w", h.FQDN, err)
		}
		ret[h.FQDN] = t
	}
	return ret, nil
}
