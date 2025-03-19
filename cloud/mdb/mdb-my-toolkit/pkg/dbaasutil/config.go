package dbaasutil

import (
	"encoding/json"
	"io/ioutil"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type DbaasConfig struct {
	ClusterID    string   `json:"cluster_id"`
	ClusterHosts []string `json:"cluster_hosts"`
}

func ReadDbaasConfig(path string) (*DbaasConfig, error) {
	if path == "" {
		path = "/etc/dbaas.conf"
	}
	data, err := ioutil.ReadFile(path)
	if err != nil {
		return nil, err
	}
	config := new(DbaasConfig)
	err = json.Unmarshal(data, config)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse %q: %w", path, err)
	}
	return config, nil
}
