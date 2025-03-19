package console

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

type ClusterResources struct {
	HostType            hosts.Role
	HostCount           int64
	ResourcePresetExtID string
	DiskSize            int64
}

type Cluster struct {
	ID          string
	ProjectID   string
	CloudType   environment.CloudType
	RegionID    string
	CreateTime  time.Time
	Name        string
	Description string
	Status      clusters.Status
	Health      clusters.Health
	Type        clusters.Type
	Version     string
	Resources   []ClusterResources
}

func NewConsoleClusterPageToken(cls []Cluster, expectedPageSize int64) clusters.ClusterPageToken {
	actualSize := int64(len(cls))

	var lastName string
	if actualSize > 0 {
		lastName = cls[actualSize-1].Name
	}

	return clusters.NewClusterPageToken(actualSize, expectedPageSize, lastName)
}

type NameValidator struct {
	Regexp         string
	MinLength      int64
	MaxLength      int64
	ForbiddenNames []string
}

func NewNameValidator(regexp string, minLength, maxLength int64) NameValidator {
	return NameValidator{
		Regexp:    regexp,
		MinLength: minLength,
		MaxLength: maxLength,
	}
}

var (
	ClusterNameValidator = NewNameValidator(
		models.DefaultClusterNamePattern,
		models.DefaultClusterNameMinLen,
		models.DefaultClusterNameMaxLen,
	)
	DatabaseNameValidator = NewNameValidator(
		models.DefaultDatabaseNamePattern,
		models.DefaultDatabaseNameMinLen,
		models.DefaultDatabaseNameMaxLen,
	)
	UserNameValidator = NewNameValidator(
		models.DefaultUserNamePattern,
		models.DefaultUserNameMinLen,
		models.DefaultUserNameMaxLen,
	)
	PasswordValidator = NewNameValidator(
		models.DefaultUserPasswordPattern,
		models.DefaultUserPasswordMinLen,
		models.DefaultUserPasswordMaxLen,
	)
)

type DiskTypeHostCount struct {
	DiskTypeID   string
	MinHostCount int64
}

type HostCountLimits struct {
	MinHostCount         int64
	MaxHostCount         int64
	HostCountPerDiskType []DiskTypeHostCount
}

type HostTypeDiskType struct {
	ID        string
	SizeRange *Int64Range
	Sizes     []int64
	MinHosts  int64
	MaxHosts  int64
}

func (dt HostTypeDiskType) extractCommon() HostTypeDiskType {
	return HostTypeDiskType{
		ID:        dt.ID,
		SizeRange: nil,
		Sizes:     nil,
		MinHosts:  dt.MinHosts,
		MaxHosts:  dt.MaxHosts,
	}
}

func (dt HostTypeDiskType) WithSizeRange(sizeRange *Int64Range) HostTypeDiskType {
	res := dt.extractCommon()
	res.SizeRange = sizeRange
	return res
}

func (dt HostTypeDiskType) WithSizes(sizes []int64) HostTypeDiskType {
	res := dt.extractCommon()
	res.Sizes = sizes
	return res
}

type HostTypeZone struct {
	ID        string
	DiskTypes []HostTypeDiskType
}

type HostTypeResourcePreset struct {
	ID             string
	CPULimit       int64
	RAMLimit       int64
	Generation     int64
	GenerationName string
	FlavorType     string
	CPUFraction    int64
	Decomissioning bool
	Zones          []HostTypeZone
}

type HostType struct {
	Type             hosts.Role
	ResourcePresets  []HostTypeResourcePreset
	DefaultResources DefaultResources
}

type ClustersConfig struct {
	ClusterNameValidator  NameValidator
	DatabaseNameValidator NameValidator
	UserNameValidator     NameValidator
	PasswordValidator     NameValidator
	HostCountLimits       HostCountLimits
	HostTypes             []HostType
}
