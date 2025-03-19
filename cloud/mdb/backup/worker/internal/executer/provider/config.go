package provider

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/backupmanager"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
)

type DeployCmdConfig struct {
	Type    string                `json:"type" yaml:"type"`
	Args    []string              `json:"args" yaml:"args"`
	Timeout encodingutil.Duration `json:"timeout" yaml:"timeout"`
}

type PlannedConfig struct {
	DeployType          backupmanager.DeployType `json:"deploy_type" yaml:"deploy_type"`
	CmdConfig           DeployCmdConfig          `json:"cmd_config" yaml:"cmd_config"`
	SyncAllStateTimeout encodingutil.Duration    `json:"sync_all_state_timeout" yaml:"sync_all_state_timeout"`
}

type CreatingConfig struct {
	CheckDeployTimeout      encodingutil.Duration `json:"check_deploy_timeout" yaml:"check_deploy_timeout"`
	UpdateSizes             bool                  `json:"update_sizes" yaml:"update_sizes"`
	AssertUpdateSizesErrors bool                  `json:"assert_update_sizes_errors" yaml:"assert_update_sizes_errors"`
}

type ObsoleteConfig struct {
	DeployType          backupmanager.DeployType `json:"deploy_type" yaml:"deploy_type"`
	CmdConfig           DeployCmdConfig          `json:"cmd_config" yaml:"cmd_config"`
	SyncAllStateTimeout encodingutil.Duration    `json:"sync_all_state_timeout" yaml:"sync_all_state_timeout"`
}

type DeletingConfig struct {
	CheckDeployTimeout      encodingutil.Duration `json:"check_deploy_timeout" yaml:"check_deploy_timeout"`
	UpdateSizes             bool                  `json:"update_sizes" yaml:"update_sizes"`
	AssertUpdateSizesErrors bool                  `json:"assert_update_sizes_errors" yaml:"assert_update_sizes_errors"`
}

type ConfigSet struct {
	ClusterTypeSettings map[metadb.ClusterType]Config `json:"cluster_type_settings" yaml:"cluster_type_settings"`
	DefaultSettings     Config                        `json:"default_settings" yaml:"default_settings"`
}

type Config struct {
	Planned  *PlannedConfig  `json:"planned,omitempty" yaml:"planned,omitempty"`
	Creating *CreatingConfig `json:"creating,omitempty" yaml:"creating,omitempty"`
	Obsolete *ObsoleteConfig `json:"obsolete,omitempty" yaml:"obsolete,omitempty"`
	Deleting *DeletingConfig `json:"deleting,omitempty" yaml:"deleting,omitempty"`
}

func DefaultConfig() ConfigSet {
	return ConfigSet{
		ClusterTypeSettings: map[metadb.ClusterType]Config{
			metadb.PostgresqlCluster: {
				Creating: &CreatingConfig{
					CheckDeployTimeout: encodingutil.Duration{
						Duration: 3 * time.Second,
					},
					UpdateSizes:             true,
					AssertUpdateSizesErrors: false,
				},
				Deleting: &DeletingConfig{
					CheckDeployTimeout: encodingutil.Duration{
						Duration: 3 * time.Second,
					},
					UpdateSizes:             true,
					AssertUpdateSizesErrors: false,
				},
			},
		},
		DefaultSettings: Config{
			Planned: &PlannedConfig{
				DeployType: backupmanager.StateDeployType,
				CmdConfig: DeployCmdConfig{
					Type: "state.sls",
					Args: []string{"components.dbaas-operations.backup", "concurrent=True", "sync_mods=states,modules"},
					Timeout: encodingutil.Duration{
						Duration: 20 * time.Hour,
					},
				},
				SyncAllStateTimeout: encodingutil.Duration{
					Duration: 600 * time.Second,
				},
			},
			Creating: &CreatingConfig{
				CheckDeployTimeout: encodingutil.Duration{
					Duration: 3 * time.Second,
				},
			},
			Obsolete: &ObsoleteConfig{
				DeployType: backupmanager.StateDeployType,
				CmdConfig: DeployCmdConfig{
					Type: "state.sls",
					Args: []string{"components.dbaas-operations.backup-delete", "concurrent=True", "sync_mods=states,modules"},
					Timeout: encodingutil.Duration{
						Duration: 20 * time.Hour,
					},
				},
				SyncAllStateTimeout: encodingutil.Duration{
					Duration: 600 * time.Second,
				},
			},
			Deleting: &DeletingConfig{
				CheckDeployTimeout: encodingutil.Duration{
					Duration: 3 * time.Second,
				},
			},
		},
	}
}

func (cs *ConfigSet) ByClusterType(cType metadb.ClusterType) Config {
	cfg, exists := cs.ClusterTypeSettings[cType]
	if !exists {
		return cs.DefaultSettings
	}

	if cfg.Planned == nil {
		cfg.Planned = cs.DefaultSettings.Planned
	}
	if cfg.Creating == nil {
		cfg.Creating = cs.DefaultSettings.Creating
	}
	if cfg.Obsolete == nil {
		cfg.Obsolete = cs.DefaultSettings.Obsolete
	}
	if cfg.Deleting == nil {
		cfg.Deleting = cs.DefaultSettings.Deleting
	}
	return cfg
}
