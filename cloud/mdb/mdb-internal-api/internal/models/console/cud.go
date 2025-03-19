package console

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

const (
	BillingSchemaNameYandexCloud string = "mdb.db.generic.v1"
	BillingSchemaNameDoubleCloud string = "mdb.db.dc.v1"
	BillingLicenseSchemaName     string = "mdb.db.license.v1"
)

func BillingSchemaForCloudType(ct environment.CloudType) string {
	switch ct {
	case environment.CloudTypeYandex:
		return BillingSchemaNameYandexCloud
	case environment.CloudTypeAWS:
		return BillingSchemaNameDoubleCloud
	default:
		panic(fmt.Sprintf("unknown cloud type: %q", ct))
	}
}

type PlatformID string

type Platform struct {
	ID          PlatformID `json:"platform_id" yaml:"platforms_id"`
	Description string     `json:"description" yaml:"description"`
	Generation  int64      `json:"generation" yaml:"generation"`
}

type UsedResources struct {
	CloudID     string
	ClusterType clusters.Type
	Role        hosts.Role
	PlatformID  PlatformID
	Cores       int64
	Memory      int64
}

type DefaultVersion struct {
	MajorVersion string
	MinorVersion string
	Name         string
	Edition      string
	IsDefault    bool
	IsDeprecated bool
	UpdatableTo  []string
}

type Version struct {
	ClusterID      optional.String
	SubClusterID   optional.String
	ShardID        optional.String
	Component      string
	MajorVersion   string
	MinorVersion   string
	PackageVersion string
	Edition        string
}
