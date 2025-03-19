package model

import (
	"encoding/json"
	"fmt"
	"time"

	"google.golang.org/genproto/googleapis/rpc/status"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb"
	"a.yandex-team.ru/cloud/marketplace/license_server/pkg/utils"
	"a.yandex-team.ru/cloud/marketplace/pkg/protob"
	ydb_pkg "a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

const (
	CreateLicenseTemplateDescription    string = "Create License Template"
	DeprecateLicenseTemplateDescription string = "Deprecate License Template"
	DeleteLicenseTemplateDescription    string = "Delete License Template"

	CreateLicenseTemplateVersionDescription string = "Create License Template Version"
	ApplyLicenseTemplateVersionDescription  string = "Apply License Template Version"
	DeleteLicenseTemplateVersionDescription string = "Delete License Template Version"

	CreateLicenseInstanceDescription    string = "Create License Instance"
	CancelLicenseInstanceDescription    string = "Cancel License Instance"
	DeleteLicenseInstanceDescription    string = "Delete License Instance"
	DeprecateLicenseInstanceDescription string = "Deprecate License Instance"

	CreateLicenseLockDescription  string = "Create License Lock"
	ReleaseLicenseLockDescription string = "Release License Lock"
)

type Operation struct {
	ID          string
	Description string
	CreatedAt   time.Time
	CreatedBy   string
	ModifiedAt  time.Time
	Done        bool
	Metadata    json.RawMessage
	Result      json.RawMessage
}

func NewOperationFromYDB(o *ydb.Operation) (*Operation, error) {
	if o == nil {
		return nil, nil
	}

	out := &Operation{
		ID:          o.ID,
		Description: o.Description,
		CreatedBy:   o.CreatedBy,
		CreatedAt:   time.Time(o.CreatedAt),
		ModifiedAt:  time.Time(o.ModifiedAt),
		Done:        o.Done,
		Metadata:    o.Metadata.Bytes(),
		Result:      o.Result.Bytes(),
	}

	return out, nil
}

func (o *Operation) Proto() (*operation.Operation, error) {
	if o == nil {
		return nil, fmt.Errorf("operation parameter is nil")
	}

	out := &operation.Operation{
		Id:          o.ID,
		Description: o.Description,
		CreatedAt:   utils.GetTimestamppbFromTime(o.CreatedAt),
		CreatedBy:   o.CreatedBy,
		ModifiedAt:  utils.GetTimestamppbFromTime(o.ModifiedAt),
		Done:        o.Done,
	}

	if o.Metadata != nil {
		metadata, err := protob.AnyMessageFromJSON(o.Metadata)
		if err != nil {
			return nil, err
		}
		out.Metadata = metadata
	}

	result, err := protob.AnyMessageFromJSON(o.Result)
	if err != nil {
		return nil, err
	}
	if result.MessageIs((*status.Status)(nil)) {
		e := new(status.Status)
		err = result.UnmarshalTo(e)
		if err != nil {
			return nil, err
		}
		out.Result = &operation.Operation_Error{Error: e}
	} else {
		out.Result = &operation.Operation_Response{Response: result}
	}

	return out, nil
}

func (o *Operation) YDB() (*ydb.Operation, error) {
	if o == nil {
		return nil, nil
	}

	out := &ydb.Operation{
		ID:          o.ID,
		Description: o.Description,
		CreatedBy:   o.CreatedBy,
		CreatedAt:   ydb_pkg.UInt64Ts(o.CreatedAt),
		ModifiedAt:  ydb_pkg.UInt64Ts(o.ModifiedAt),
		Done:        o.Done,
		Metadata:    ydb_pkg.AnyJSON(o.Metadata),
		Result:      ydb_pkg.AnyJSON(o.Result),
	}

	return out, nil
}
