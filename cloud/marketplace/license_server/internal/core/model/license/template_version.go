package license

import (
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb"
	"a.yandex-team.ru/cloud/marketplace/license_server/pkg/utils"
	ydb_pkg "a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

type TemplateVersionState string

const (
	PendingTemplateVersionState    TemplateVersionState = "pending"
	ActiveTemplateVersionState     TemplateVersionState = "active"
	DeprecatedTemplateVersionState TemplateVersionState = "deprecated"
	DeletedTemplateVersionState    TemplateVersionState = "deleted"
)

type TemplateVersion struct {
	ID           string               `json:"id"`
	TemplateID   string               `json:"license_template_id"`
	Price        model.Price          `json:"price"`
	LicenseSkuID string               `json:"license_sku_id"`
	Name         string               `json:"name"`
	Period       model.Period         `json:"period"`
	CreatedAt    time.Time            `json:"created_at"`
	UpdatedAt    time.Time            `json:"updated_at"`
	State        TemplateVersionState `json:"state"`
}

func NewTemplateVersionFromYDB(ltv *ydb.LicenseTemplateVersion) (*TemplateVersion, error) {
	if ltv == nil {
		return nil, nil
	}

	period, err := model.NewPeriod(string(ltv.Period))
	if err != nil {
		return nil, err
	}

	out := &TemplateVersion{
		ID:           ltv.ID,
		TemplateID:   ltv.TemplateID,
		Price:        model.Price(ltv.Price),
		LicenseSkuID: string(ltv.LicenseSkuID),
		Name:         ltv.Name,
		Period:       period,
		CreatedAt:    time.Time(ltv.CreatedAt),
		UpdatedAt:    time.Time(ltv.UpdatedAt),
	}

	switch ltv.State {
	case "active":
		out.State = ActiveTemplateVersionState
	case "pending":
		out.State = PendingTemplateVersionState
	case "deprecated":
		out.State = DeprecatedTemplateVersionState
	case "deleted":
		out.State = DeletedTemplateVersionState
	}

	return out, nil
}

func (ltv *TemplateVersion) Proto() (*license.TemplateVersion, error) {
	if ltv == nil {
		return nil, fmt.Errorf("license template parameter is nil")
	}

	out := &license.TemplateVersion{
		Id:           ltv.ID,
		TemplateId:   ltv.TemplateID,
		Name:         ltv.Name,
		Prices:       &license.Price{CurrencyToValue: ltv.Price},
		LicenseSkuId: ltv.LicenseSkuID,
		Period:       ltv.Period.String(),
		CreatedAt:    utils.GetTimestamppbFromTime(ltv.CreatedAt),
		UpdatedAt:    utils.GetTimestamppbFromTime(ltv.UpdatedAt),
	}

	switch ltv.State {
	case ActiveTemplateVersionState:
		out.State = license.TemplateVersion_ACTIVE
	case PendingTemplateVersionState:
		out.State = license.TemplateVersion_PENDING
	case DeprecatedTemplateVersionState:
		out.State = license.TemplateVersion_DEPRECATED
	case DeletedTemplateVersionState:
		out.State = license.TemplateVersion_DELETED
	default:
		out.State = license.TemplateVersion_STATE_UNSPECIFIED
	}

	return out, nil
}

func TemplateVersionsListToProto(lts []*TemplateVersion) ([]*license.TemplateVersion, error) {
	if lts == nil {
		return nil, nil
	}

	outs := make([]*license.TemplateVersion, 0, len(lts))
	for i := range lts {
		out, err := lts[i].Proto()
		if err != nil {
			return nil, err
		}
		outs = append(outs, out)
	}

	return outs, nil
}

func (ltv *TemplateVersion) YDB() (*ydb.LicenseTemplateVersion, error) {
	if ltv == nil {
		return nil, nil
	}

	out := &ydb.LicenseTemplateVersion{
		ID:           ltv.ID,
		TemplateID:   ltv.TemplateID,
		Price:        ydb_pkg.MapStringStringJSON(ltv.Price),
		LicenseSkuID: ydb_pkg.String(ltv.LicenseSkuID),
		Name:         ltv.Name,
		Period:       ltv.Period.String(),
		CreatedAt:    ydb_pkg.UInt64Ts(ltv.CreatedAt),
		UpdatedAt:    ydb_pkg.UInt64Ts(ltv.UpdatedAt),
		State:        string(ltv.State),
	}

	return out, nil
}
