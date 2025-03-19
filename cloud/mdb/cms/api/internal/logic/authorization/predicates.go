package authorization

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/authentication"
)

const WalleID = 2010518

// tvm client id с которым приходит Wall-e в ЦМС
var walleSystemServiceIDs = []uint32{WalleID}

func IsAuthenticated(ctx context.Context, result authentication.Result) bool {
	return result.IsAuthenticated()
}

func IsWalleSystem(ctx context.Context, result authentication.Result) bool {
	for _, a := range walleSystemServiceIDs {
		if a == result.ServiceID() {
			return true
		}
	}
	return false
}
