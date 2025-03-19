package api

import (
	"encoding/base64"
	"encoding/json"
	"strconv"

	"github.com/golang/protobuf/ptypes/wrappers"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// TODO: replace it with ParsePageTokenFromGRPC
func PageTokenFromGRPC(pageToken string) (int64, bool, error) {
	if pageToken == "" {
		return 0, false, nil
	}

	token, err := strconv.ParseInt(pageToken, 10, 64)
	if err != nil {
		e := semerr.InvalidInput("invalid page token")
		return 0, false, xerrors.Errorf("%w: %s", e, err)
	}

	return token, true, nil
}

// TODO: replace it with BuildPageTokenToGRPC
func PagingTokenToGRPC(pageToken int64) string {
	return strconv.FormatInt(pageToken, 10)
}

func ParsePageTokenFromGRPC(token string, pageToken pagination.PageToken) error {
	if token == "" {
		return nil
	}
	data, err := base64.StdEncoding.DecodeString(token)
	if err != nil {
		return semerr.InvalidInput("invalid page token")
	}
	return json.Unmarshal(data, pageToken)
}

func BuildPageTokenToGRPC(pageToken pagination.PageToken, always bool) (string, error) {
	if !(pageToken.HasMore() || always) {
		return "", nil
	}
	data, err := json.Marshal(pageToken)
	if err != nil {
		return "", err
	}
	return base64.StdEncoding.EncodeToString(data), nil
}

func WrapInt64(val int64) *wrappers.Int64Value {
	if val == 0 {
		return nil
	}
	return &wrappers.Int64Value{Value: val}
}

func WrapInt64ToInt64Pointer(val int64) *int64 {
	return &val
}

func WrapInt64Pointer(val *int64) *wrappers.Int64Value {
	if val == nil {
		return nil
	}
	return &wrappers.Int64Value{Value: *val}
}

func WrapBool(val bool) *wrappers.BoolValue {
	if !val {
		return nil
	}
	return &wrappers.BoolValue{Value: val}
}

func WrapBoolPointer(val *bool) *wrappers.BoolValue {
	if val == nil {
		return nil
	}
	return &wrappers.BoolValue{Value: *val}
}

func WrapDouble(val float64) *wrappers.DoubleValue {
	return &wrappers.DoubleValue{Value: val}
}

func WrapDoublePointer(val *float64) *wrappers.DoubleValue {
	if val == nil {
		return nil
	}
	return &wrappers.DoubleValue{Value: *val}
}

func UnwrapInt64Value(val *wrappers.Int64Value) *int64 {
	if val == nil {
		return nil
	}
	return &val.Value
}

func UnwrapBoolValue(val *wrappers.BoolValue) *bool {
	if val == nil {
		return nil
	}
	return &val.Value
}

func UnwrapInt64PointerToOptional(val *wrappers.Int64Value) optional.Int64Pointer {
	if val == nil {
		return optional.Int64Pointer{}
	}
	return optional.NewInt64Pointer(&val.Value)
}
