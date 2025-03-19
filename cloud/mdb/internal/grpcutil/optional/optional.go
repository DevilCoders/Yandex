package optional

import (
	"github.com/golang/protobuf/ptypes/wrappers"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

//TODO move others from https://a.yandex-team.ru/arc_vcs/cloud/mdb/mdb-internal-api/internal/api/grpc/models.go

func StringFromGRPC(v *wrappers.StringValue) optional.String {
	if v == nil {
		return optional.String{}
	}
	return optional.NewString(v.GetValue())
}

func StringToGRPC(v optional.String) *wrappers.StringValue {
	if !v.Valid {
		return nil
	}
	return &wrappers.StringValue{Value: v.Must()}
}
