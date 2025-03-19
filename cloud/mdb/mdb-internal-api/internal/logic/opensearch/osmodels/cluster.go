package osmodels

import (
	"regexp"
	"strconv"
	"strings"

	"github.com/mitchellh/copystructure"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	OpenSearchAllowUnlimitedHostsFeatureFlag string = "MDB_OPENSEARCH_ALLOW_UNLIMITED_HOSTS"
)

type Host struct {
	ZoneID         string
	SubnetID       optional.String
	AssignPublicIP bool
	Role           hosts.Role
	ShardName      string
}

type ClusterHosts struct {
	DataNodes   []Host
	MasterNodes []Host
}

type DataNodeSpec struct {
	Resources models.ClusterResourcesSpec
	Config    DataNodeConfig
}

func (cs *DataNodeSpec) Validate(allResourcesRequired bool) error {
	return combineValidationErrors(
		cs.Config.Validate(),
		cs.Resources.Validate(allResourcesRequired),
	)
}

type DataNode struct {
	Resources models.ClusterResources
	ConfigSet DataNodeConfigSet
}

type MasterNodeSpec struct {
	Resources models.ClusterResourcesSpec
}

func (cs *MasterNodeSpec) Validate(allResourcesRequired bool) error {
	return cs.Resources.Validate(allResourcesRequired)
}

type MasterNode struct {
	Resources models.ClusterResources
}

type DataNodeConfig struct {
	FielddataCacheSize     optional.String `json:"fielddata_cache_size"`
	MaxClauseCount         optional.Int64  `json:"max_clause_count"`
	ReindexRemoteWhitelist optional.String `json:"reindex_remote_whitelist"`
	ReindexSSLCAPath       optional.String `json:"reindex_ssl_ca_path"`
}

func (c *DataNodeConfig) Merge(other *DataNodeConfig) (DataNodeConfig, error) {
	mergeResult, err := copystructure.Copy(c)
	if err != nil {
		return DataNodeConfig{}, err
	}
	reflectutil.MergeStructs(mergeResult, other, reflectutil.ValidOptionalFieldsAreValid)
	return *mergeResult.(*DataNodeConfig), nil
}

func (c *DataNodeConfig) MarshalJSON() ([]byte, error) {
	return optional.MarshalStructWithOnlyOptionalFields(c)
}

func (c *DataNodeConfig) UnmarshalJSON(b []byte) (err error) {
	return optional.UnmarshalToStructWithOnlyOptionalFields(b, c)
}

var sizeRegexp = regexp.MustCompile(`^\d+([kKmMgGtTpP]?[bB]|[kKmMgGtTpP][bB]?)$`)

func validateSizeValue(value, name string) error {
	isValid := true
	if strings.HasSuffix(value, "%") {
		i, err := strconv.ParseInt(value[:len(value)-1], 10, 64)
		if err != nil || (i < 0 || i > 100) {
			isValid = false
		}
	} else {
		if !sizeRegexp.MatchString(value) {
			isValid = false
		}
	}
	if isValid {
		return nil
	}
	return xerrors.Errorf("incorrect value for %s", name)
}

func validateInt64RangeValue(value, min, max int64, name string) error {
	if value < min {
		return xerrors.Errorf(
			"specified value for %s is too small (min: %d, max: %d)", name, min, max)
	}
	if value > max {
		return xerrors.Errorf(
			"specified value for %s is too large (min: %d, max: %d)", name, min, max)
	}
	return nil
}

func combineValidationErrors(errors ...error) error {
	var filtered []error
	for _, e := range errors {
		if e != nil {
			filtered = append(filtered, e)
		}
	}
	if len(filtered) > 0 {
		err := filtered[0]
		for _, e := range filtered[1:] {
			err = xerrors.Errorf("%s;%s", err, e)
		}
		return err
	}
	return nil
}

func (c *DataNodeConfig) Validate() error {
	var err error
	var errors []error

	if c.FielddataCacheSize.Valid {
		err = validateSizeValue(c.FielddataCacheSize.String, "indices.fielddata.cache.size")
		if err != nil {
			errors = append(errors, err)
		}
	}
	if c.MaxClauseCount.Valid {
		// the upper bound equals to default * 10
		err = validateInt64RangeValue(c.MaxClauseCount.Int64, 0, 1024*10, "indices.query.bool.max_clause_count")
		if err != nil {
			errors = append(errors, err)
		}
	}
	return combineValidationErrors(errors...)
}

type OptionalMasterNodeSpec struct {
	Value MasterNodeSpec
	Valid bool
}

type OptionalMasterNode struct {
	Value MasterNode
	Valid bool
}

type OptionalDataNodeSpec struct {
	Value DataNodeSpec
	Valid bool
}

type OptionalPlugins struct {
	Value []string
	Valid bool
}

func (o *OptionalPlugins) Set(value []string) {
	o.Value = value
	o.Valid = true
}

func (o *OptionalDataNodeSpec) Set(v DataNodeSpec) {
	o.Value = v
	o.Valid = true
}

func (o *OptionalDataNodeSpec) Get() (DataNodeSpec, error) {
	if !o.Valid {
		return DataNodeSpec{}, optional.ErrMissing
	}
	return o.Value, nil
}

func (o *OptionalMasterNodeSpec) Set(v MasterNodeSpec) {
	o.Value = v
	o.Valid = true
}

func (o *OptionalMasterNodeSpec) Get() (MasterNodeSpec, error) {
	if !o.Valid {
		return MasterNodeSpec{}, optional.ErrMissing
	}
	return o.Value, nil
}

func (o *OptionalMasterNode) Set(v MasterNode) {
	o.Value = v
	o.Valid = true
}

func (o *OptionalMasterNode) Get() (MasterNode, error) {
	if !o.Valid {
		return MasterNode{}, optional.ErrMissing
	}
	return o.Value, nil
}

type ConfigSpec struct {
	MasterNode OptionalMasterNodeSpec
	DataNode   DataNodeSpec
	Plugins    []string
}

type Config struct {
	MasterNode OptionalMasterNode
	DataNode   DataNode
	Plugins    []string
}

type OptionalConfigSpec struct {
	Value ConfigSpec
	Valid bool
}

func (o *OptionalConfigSpec) Set(v ConfigSpec) {
	o.Value = v
	o.Valid = true
}

func (o *OptionalConfigSpec) Get() (ConfigSpec, error) {
	if !o.Valid {
		return ConfigSpec{}, optional.ErrMissing
	}
	return o.Value, nil
}

type OptionalVersion struct {
	Value Version
	Valid bool
}

func NewOptionalVersion(v Version) OptionalVersion {
	return OptionalVersion{Value: v, Valid: true}
}

func (o *OptionalVersion) Set(v Version) {
	o.Value = v
	o.Valid = true
}

func (o *OptionalVersion) Get() (Version, error) {
	if !o.Valid {
		return Version{}, optional.ErrMissing
	}
	return o.Value, nil
}

func (o *OptionalVersion) GetDefault(def Version) Version {
	if !o.Valid {
		return def
	}
	return o.Value
}

type OptionalSecretString struct {
	Value secret.String
	Valid bool
}

func (o *OptionalSecretString) Set(v string) {
	o.Value = secret.NewString(v)
	o.Valid = true
}

func (o *OptionalSecretString) Get() (secret.String, error) {
	if !o.Valid {
		return secret.String{}, optional.ErrMissing
	}
	return o.Value, nil
}

type OptionalAccess struct {
	Value clusters.Access
	Valid bool
}

func (o *OptionalAccess) Set(v clusters.Access) {
	o.Value = v
	o.Valid = true
}

func (o *OptionalAccess) Get() (clusters.Access, error) {
	if !o.Valid {
		return clusters.Access{}, optional.ErrMissing
	}
	return o.Value, nil
}

type ClusterConfigSpec struct {
	Version       OptionalVersion
	Config        OptionalConfigSpec
	AdminPassword OptionalSecretString
	Access        OptionalAccess
}

type ClusterConfig struct {
	Version       Version
	Config        Config
	AdminPassword OptionalSecretString
	Access        clusters.Access
}

type Cluster struct {
	clusters.ClusterExtended
	ServiceAccountID string
	Config           ClusterConfig
}

type OpenSearchSpecUpdate struct {
	MasterNode OptionalMasterNodeSpec
	DataNode   OptionalDataNodeSpec
	Plugins    OptionalPlugins
}

type ConfigSpecUpdate struct {
	Version       OptionalVersion
	OpenSearch    OpenSearchSpecUpdate
	AdminPassword OptionalSecretString
	Access        OptionalAccess
}

type DataNodeConfigSet struct {
	EffectiveConfig DataNodeConfig
	DefaultConfig   DataNodeConfig
	UserConfig      DataNodeConfig
}
