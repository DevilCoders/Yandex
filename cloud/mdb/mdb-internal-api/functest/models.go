package functest

import (
	"fmt"
	"os"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type API int

func (a API) String() string {
	switch a {
	case APIInvalid:
		return "invalid"
	case APIREST:
		return "rest"
	case APIGRPC:
		return "grpc"
	default:
		panic(fmt.Sprintf("unknown API: %d", a))
	}
}

const (
	APIInvalid API = iota
	APIREST
	APIGRPC
	APIDATACLOUD
	APIPILLARCONFIG
)

type Mode int

func (m Mode) String() string {
	switch m {
	case ModeInvalid:
		return "invalid"
	case ModeRead:
		return "read"
	case ModeModify:
		return "modify"
	default:
		panic(fmt.Sprintf("unknown mode: %d", m))
	}
}

const (
	ModeInvalid Mode = iota
	ModeRead
	ModeModify
)

func modeFromRESTMethod(method string) (Mode, error) {
	switch method {
	case "GET":
		return ModeRead, nil
	case "POST", "PUT", "PATCH", "DELETE":
		return ModeModify, nil
	default:
		return ModeInvalid, xerrors.Errorf("unknown REST method: %s", method)
	}
}

func modeFromGRPCMethod(method string) Mode {
	if interceptors.MethodReadOnly(method, api.YandexCloudReadOnlyMethods) {
		return ModeRead
	}

	return ModeModify
}

const (
	EnvNameModifyModeAPI = "MDB_INTERNAL_API_FUNC_TEST_MODIFY_MODE_API"
	EnvNameReadModeAPI   = "MDB_INTERNAL_API_FUNC_TEST_READ_MODE_API"
)

func parseAPI(s string) (API, error) {
	switch s {
	case "rest":
		return APIREST, nil
	case "grpc":
		return APIGRPC, nil
	default:
		return APIInvalid, xerrors.Errorf("invalid api name: %s", s)
	}
}

func modifyModeAPIFromEnv() (API, error) {
	return apiFromEnv(EnvNameModifyModeAPI)
}

func readModeAPIFromEnv() (API, error) {
	return apiFromEnv(EnvNameReadModeAPI)
}

func apiFromEnv(env string) (API, error) {
	api, ok := os.LookupEnv(env)
	if !ok {
		return APIInvalid, xerrors.Errorf("env variable %s is not set", env)
	}

	return parseAPI(api)
}

const (
	ListMethodPrefix = "List"
)

func isListMethodPrefix(method string) bool {
	return strings.HasPrefix(method, ListMethodPrefix)
}
