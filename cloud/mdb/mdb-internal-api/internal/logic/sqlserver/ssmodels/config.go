package ssmodels

import (
	"fmt"
	"math"
	"strings"

	"github.com/mitchellh/copystructure"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
)

const (
	Version2016sp2dev = "2016sp2dev"
	Version2016sp2std = "2016sp2std"
	Version2016sp2ent = "2016sp2ent"
	Version2017dev    = "2017dev"
	Version2017std    = "2017std"
	Version2017ent    = "2017ent"
	Version2018sp2std = "2018sp2std"
	Version2018sp2ent = "2018sp2ent"
	Version2019dev    = "2019dev"
	Version2019std    = "2019std"
	Version2019ent    = "2019ent"
	DefaultVersion    = Version2016sp2ent
)

const (
	StandardEditionMaxCores  = 24
	StandaddEditionMaxMemory = 128 * 1024 * 1024 * 1024
	MinCoresToBill           = 4
)

var Versions []clusters.Version = []clusters.Version{
	{ID: Version2016sp2dev, Name: "2016 ServicePack 2 Developer Edition"},
	{ID: Version2016sp2std, Name: "2016 ServicePack 2 Standard Edition"},
	{ID: Version2016sp2ent, Name: "2016 ServicePack 2 Enterprise Edition"},
	{ID: Version2017dev, Name: "2017 Developer Edition"},
	{ID: Version2017std, Name: "2017 Standard Edition"},
	{ID: Version2017ent, Name: "2017 Enterprise Edition"},
	{ID: Version2019dev, Name: "2019 Developer Edition"},
	{ID: Version2019std, Name: "2019 Standard Edition"},
	{ID: Version2019ent, Name: "2019 Enterprise Edition"},
}

const (
	EditionEnterprise = "enterprise"
	EditionStandard   = "standard"
	EditionDeveloper  = "developer"
)

func VersionEdition(v string) string {
	switch {
	case strings.HasSuffix(v, "ent"):
		return EditionEnterprise
	case strings.HasSuffix(v, "std"):
		return EditionStandard
	case strings.HasSuffix(v, "dev"):
		return EditionDeveloper
	default:
		panic(fmt.Sprintf("invalid version string: %s", v))
	}
}

type SQLServerConfig interface {
	Version() clusters.Version
	Validate(preset resources.Preset) error
}

type ConfigBase struct {
	MaxDegreeOfParallelism      optional.Int64 `json:"max_degree_of_parallelism"`
	CostThresholdForParallelism optional.Int64 `json:"cost_threshold_for_parallelism"`
	AuditLevel                  optional.Int64 `json:"audit_level"`
	FillFactorPercent           optional.Int64 `json:"fill_factor_percent"`
	OptimizeForAdHocWorkloads   optional.Bool  `json:"optimize_for_ad_hoc_workloads"`
}

func (c *ConfigBase) Version() clusters.Version {
	return Versions[0]
}

func (c *ConfigBase) MarshalJSON() ([]byte, error) {
	return optional.MarshalStructWithOnlyOptionalFields(c)
}

func (c *ConfigBase) UnmarshalJSON(b []byte) (err error) {
	return optional.UnmarshalToStructWithOnlyOptionalFields(b, c)
}

func (c *ConfigBase) Validate(rs resources.Preset) error {
	if c.MaxDegreeOfParallelism.Valid {
		maxDegreeOfParallelism, err := c.MaxDegreeOfParallelism.Get()
		if err == nil && (maxDegreeOfParallelism < 0 || maxDegreeOfParallelism > math.MaxInt16) {
			return semerr.InvalidInputf("MaxDegreeOfParallelism must be in range from 0 to 32767 instead of: %d", maxDegreeOfParallelism)
		}
	}
	if c.CostThresholdForParallelism.Valid {
		costThresholdForParallelism, err := c.CostThresholdForParallelism.Get()
		if err == nil && (costThresholdForParallelism < 5 || costThresholdForParallelism > math.MaxInt16) {
			return semerr.InvalidInputf("CostThresholdForParallelism must be in range from 5 to 32767 instead of: %d", costThresholdForParallelism)
		}
	}
	if c.AuditLevel.Valid {
		auditLevel, err := c.AuditLevel.Get()
		if err == nil && (auditLevel < 0 || auditLevel > 3) {
			return semerr.InvalidInputf("AuditLevel must be in range from 0 to 3 instead of: %d", auditLevel)
		}
	}
	if c.FillFactorPercent.Valid {
		fillFactorPercent, err := c.FillFactorPercent.Get()
		if err == nil && (fillFactorPercent < 0 || fillFactorPercent > 100) {
			return semerr.InvalidInputf("FillFactorPercent must be in range from 0 to 100 instead of: %d", fillFactorPercent)
		}
	}
	return nil
}

func ClusterConfigMerge(base SQLServerConfig, add SQLServerConfig) (SQLServerConfig, error) {
	mergeResult, err := copystructure.Copy(base)
	if err != nil {
		return nil, err
	}
	reflectutil.MergeStructs(mergeResult, add, reflectutil.ValidOptionalFieldsAreValid)
	return mergeResult.(SQLServerConfig), nil
}

func ClusterConfigMergeByFields(base SQLServerConfig, add SQLServerConfig, fields []string) (SQLServerConfig, error) {
	mergeResult, err := copystructure.Copy(base)
	if err != nil {
		return nil, err
	}
	reflectutil.MergeStructs(mergeResult, add, reflectutil.FieldsInListAreValid(fields))
	return mergeResult.(SQLServerConfig), nil
}

type ConfigSetSQLServer struct {
	EffectiveConfig SQLServerConfig
	DefaultConfig   SQLServerConfig
	UserConfig      SQLServerConfig
}

type Access struct {
	DataLens     bool
	WebSQL       bool
	DataTransfer bool
}

type SecondaryConnectionsMode int

const (
	SecondaryConnectionsOff      SecondaryConnectionsMode = 0
	SecondaryConnectionsReadOnly SecondaryConnectionsMode = 1
)

func SecondaryConnectionsToUR(sc SecondaryConnectionsMode) bool {
	return sc == SecondaryConnectionsOff
}

func SecondaryConnectionsFromUR(ur bool) SecondaryConnectionsMode {
	if ur {
		return SecondaryConnectionsOff
	}
	return SecondaryConnectionsReadOnly
}

type OptionalSecondaryConnectionsMode struct {
	Value SecondaryConnectionsMode
	Valid bool
}

func (osc *OptionalSecondaryConnectionsMode) Get() (SecondaryConnectionsMode, error) {
	if !osc.Valid {
		return SecondaryConnectionsOff, optional.ErrMissing
	}
	return osc.Value, nil
}

func (osc *OptionalSecondaryConnectionsMode) GetOrDefault() SecondaryConnectionsMode {
	if !osc.Valid {
		return SecondaryConnectionsOff
	}
	return osc.Value
}

func (osc *OptionalSecondaryConnectionsMode) Set(sc SecondaryConnectionsMode) {
	osc.Value = sc
	osc.Valid = true
}

type ClusterConfig struct {
	Version              string
	ConfigSet            ConfigSetSQLServer
	Resources            models.ClusterResources
	BackupWindowStart    bmodels.BackupWindowStart
	SecondaryConnections SecondaryConnectionsMode
	Access               Access
}

type AccessSpec struct {
	DataLens     optional.Bool
	WebSQL       optional.Bool
	DataTransfer optional.Bool
}

type ClusterConfigSpec struct {
	Version              optional.String
	Config               OptionalSQLServerConfig
	Resources            models.ClusterResourcesSpec
	BackupWindowStart    bmodels.OptionalBackupWindowStart
	SecondaryConnections OptionalSecondaryConnectionsMode
	Access               AccessSpec
}

func (cs ClusterConfigSpec) Validate(rs resources.Preset, allRequired bool) error {
	if allRequired || cs.Version.Valid {
		if !cs.Version.Valid || cs.Version.Must() == "" {
			return semerr.InvalidInput("version must be specified")
		}
	}
	if cs.Config.Valid {
		if cs.Config.Value == nil {
			return semerr.InvalidInput("config must be specified")
		}
	}
	if err := cs.Resources.Validate(allRequired); err != nil {
		return err
	}
	// TODO: config validation against flavour
	if cs.Config.Valid {
		err := cs.Config.Value.Validate(rs)
		if err != nil {
			return err
		}
	}
	if cs.BackupWindowStart.Valid {
		err := cs.BackupWindowStart.Value.Validate()
		if err != nil {
			return err
		}
	}
	return nil
}

type OptionalSQLServerConfig struct {
	Value  SQLServerConfig
	Valid  bool
	Fields []string
}

func NewOptionalSQLServerConfig(v SQLServerConfig, fields []string) OptionalSQLServerConfig {
	return OptionalSQLServerConfig{Value: v, Valid: true, Fields: fields}
}
