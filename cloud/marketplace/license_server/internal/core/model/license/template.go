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

type TemplateState string

const (
	PendingTemplateState    TemplateState = "pending"
	ActiveTemplateState     TemplateState = "active"
	DeprecatedTemplateState TemplateState = "deprecated"
	DeletedTemplateState    TemplateState = "deleted"
)

type Template struct {
	ID                string        `json:"id"`
	TemplateVersionID string        `json:"license_template_version_id"`
	Name              string        `json:"name"`
	PublisherID       string        `json:"publisher_id"`
	ProductID         string        `json:"product_id"`
	TariffID          string        `json:"tariff_id"`
	LicenseSkuID      string        `json:"license_sku_id"`
	Period            model.Period  `json:"period"`
	CreatedAt         time.Time     `json:"created_at"`
	UpdatedAt         time.Time     `json:"updated_at"`
	State             TemplateState `json:"state"`
}

func NewTemplateFromYDB(lt *ydb.LicenseTemplate) (*Template, error) {
	if lt == nil {
		return nil, nil
	}

	period, err := model.NewPeriod(string(lt.Period))
	if err != nil {
		return nil, err
	}

	out := &Template{
		ID:                lt.ID,
		TemplateVersionID: string(lt.TemplateVersionID),
		PublisherID:       lt.PublisherID,
		ProductID:         lt.ProductID,
		TariffID:          lt.TariffID,
		LicenseSkuID:      string(lt.LicenseSkuID),
		Name:              string(lt.Name),
		Period:            period,
		CreatedAt:         time.Time(lt.CreatedAt),
		UpdatedAt:         time.Time(lt.CreatedAt),
	}

	switch lt.State {
	case "active":
		out.State = ActiveTemplateState
	case "pending":
		out.State = PendingTemplateState
	case "deprecated":
		out.State = DeprecatedTemplateState
	case "deleted":
		out.State = DeletedTemplateState
	}

	return out, nil
}

func (lt *Template) Proto() (*license.Template, error) {
	if lt == nil {
		return nil, fmt.Errorf("license template parameter is nil")
	}

	out := &license.Template{
		Id:          lt.ID,
		VersionId:   lt.TemplateVersionID,
		PublisherId: lt.PublisherID,
		ProductId:   lt.ProductID,
		TariffId:    lt.TariffID,
		Name:        lt.Name,
		Period:      lt.Period.String(),
		CreatedAt:   utils.GetTimestamppbFromTime(lt.CreatedAt),
		UpdatedAt:   utils.GetTimestamppbFromTime(lt.UpdatedAt),
	}

	switch lt.State {
	case ActiveTemplateState:
		out.State = license.Template_ACTIVE
	case PendingTemplateState:
		out.State = license.Template_PENDING
	case DeprecatedTemplateState:
		out.State = license.Template_DEPRECATED
	case DeletedTemplateState:
		out.State = license.Template_DELETED
	default:
		out.State = license.Template_STATE_UNSPECIFIED
	}

	return out, nil
}

func TemplatesListToProto(lts []*Template) ([]*license.Template, error) {
	if lts == nil {
		return nil, nil
	}

	outs := make([]*license.Template, 0, len(lts))
	for i := range lts {
		out, err := lts[i].Proto()
		if err != nil {
			return nil, err
		}
		outs = append(outs, out)
	}

	return outs, nil
}

func (lt *Template) YDB() (*ydb.LicenseTemplate, error) {
	if lt == nil {
		return nil, nil
	}

	out := &ydb.LicenseTemplate{
		ID:                lt.ID,
		TemplateVersionID: ydb_pkg.String(lt.TemplateVersionID),
		PublisherID:       lt.PublisherID,
		ProductID:         lt.ProductID,
		TariffID:          lt.TariffID,
		LicenseSkuID:      ydb_pkg.String(lt.LicenseSkuID),
		Name:              ydb_pkg.String(lt.Name),
		Period:            ydb_pkg.String(lt.Period.String()),
		CreatedAt:         ydb_pkg.UInt64Ts(lt.CreatedAt),
		UpdatedAt:         ydb_pkg.UInt64Ts(lt.UpdatedAt),
		State:             string(lt.State),
	}

	return out, nil
}
