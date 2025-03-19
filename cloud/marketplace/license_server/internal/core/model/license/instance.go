package license

import (
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb"
	"a.yandex-team.ru/cloud/marketplace/license_server/pkg/utils"
	ydb_pkg "a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

type InstanceState string

const (
	PendingInstanceState    InstanceState = "pending"
	ActiveInstanceState     InstanceState = "active"
	CancelledInstanceState  InstanceState = "cancelled"
	ExpiredInstanceState    InstanceState = "expired"
	DeletedInstanceState    InstanceState = "deleted"
	DeprecatedInstanceState InstanceState = "deprecated"
)

type Instance struct {
	ID                string        `json:"id"`
	TemplateID        string        `json:"template"`
	TemplateVersionID string        `json:"template_version_id"`
	CloudID           string        `json:"cloud_id"`
	Name              string        `json:"name"`
	StartTime         time.Time     `json:"start_time"`
	EndTime           time.Time     `json:"end_time"`
	CreatedAt         time.Time     `json:"created_at"`
	UpdatedAt         time.Time     `json:"updated_at"`
	State             InstanceState `json:"state"`
}

func NewInstanceFromYDB(li *ydb.LicenseInstance) (*Instance, error) {
	if li == nil {
		return nil, nil
	}

	out := &Instance{
		ID:                li.ID,
		TemplateID:        li.TemplateID,
		TemplateVersionID: li.TemplateVersionID,
		CloudID:           li.CloudID,
		Name:              li.Name,
		StartTime:         time.Time(li.StartTime),
		EndTime:           time.Time(li.EndTime),
		CreatedAt:         time.Time(li.CreatedAt),
		UpdatedAt:         time.Time(li.UpdatedAt),
	}

	switch li.State {
	case "active":
		out.State = ActiveInstanceState
	case "cancelled":
		out.State = CancelledInstanceState
	case "expired":
		out.State = ExpiredInstanceState
	case "deprecated":
		out.State = DeprecatedInstanceState
	case "deleted":
		out.State = DeletedInstanceState
	}

	return out, nil
}

func (li *Instance) Proto() (*license.Instance, error) {
	if li == nil {
		return nil, fmt.Errorf("license template parameter is nil")
	}

	out := &license.Instance{
		Id:                li.ID,
		TemplateId:        li.TemplateID,
		TemplateVersionId: li.TemplateVersionID,
		CloudId:           li.CloudID,
		Name:              li.Name,
		StartTime:         utils.GetTimestamppbFromTime(li.StartTime),
		EndTime:           utils.GetTimestamppbFromTime(li.EndTime),
		CreatedAt:         utils.GetTimestamppbFromTime(li.CreatedAt),
		UpdatedAt:         utils.GetTimestamppbFromTime(li.UpdatedAt),
	}

	switch li.State {
	case ActiveInstanceState:
		out.State = license.Instance_ACTIVE
	case CancelledInstanceState:
		out.State = license.Instance_CANCELLED
	case DeletedInstanceState:
		out.State = license.Instance_DELETED
	case DeprecatedInstanceState:
		out.State = license.Instance_DEPRECATED
	case ExpiredInstanceState:
		out.State = license.Instance_EXPIRED
	default:
		out.State = license.Instance_STATE_UNSPECIFIED
	}

	return out, nil
}

func (li *Instance) YDB() (*ydb.LicenseInstance, error) {
	if li == nil {
		return nil, nil
	}

	out := &ydb.LicenseInstance{
		ID:                li.ID,
		TemplateID:        li.TemplateID,
		TemplateVersionID: li.TemplateVersionID,
		CloudID:           li.CloudID,
		Name:              li.Name,
		StartTime:         ydb_pkg.UInt64Ts(li.StartTime),
		EndTime:           ydb_pkg.UInt64Ts(li.EndTime),
		CreatedAt:         ydb_pkg.UInt64Ts(li.CreatedAt),
		UpdatedAt:         ydb_pkg.UInt64Ts(li.UpdatedAt),
		State:             string(li.State),
	}

	return out, nil
}
