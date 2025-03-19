package swagger

import (
	"context"
	"fmt"
	"net/http"
	"reflect"
	"runtime"
	"strings"

	"github.com/go-openapi/runtime/middleware"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	swagmodels "a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func getFunctionName(function interface{}) string {
	nameParts := strings.Split(runtime.FuncForPC(reflect.ValueOf(function).Pointer()).Name(), ".")
	return nameParts[len(nameParts)-1]
}

type swagError interface {
	middleware.Responder

	SetStatusCode(code int)
	SetPayload(payload *swagmodels.Error)
}

var noSkipSentryForSemerrs = []semerr.Semantic{
	semerr.SemanticUnknown,
	semerr.SemanticInternal,
}

func skipSentryForSemantic(s semerr.Semantic) bool {
	for _, semcode := range noSkipSentryForSemerrs {
		if semcode == s {
			return false
		}
	}
	return true
}

func shouldSkipSentry(err error) bool {
	e := semerr.AsSemanticError(err)
	if e == nil {
		return false
	}
	return skipSentryForSemantic(e.Semantic)
}

var sentinelToCodeMap = map[*xerrors.Sentinel]int{}

func sentinelToCode(err error) (int, bool) {
	for sent, code := range sentinelToCodeMap {
		if xerrors.Is(err, sent) {
			return code, true
		}
	}
	return 0, false
}

func (api *API) handleError(ctx context.Context, apiHandlerFunc interface{}, se swagError, err error) swagError {
	ctxlog.Error(ctx, api.logger, fmt.Sprintf("%s error", getFunctionName(apiHandlerFunc)), log.Error(err))

	code, ok := httputil.CodeFromSemanticError(err)
	if !ok {
		if xerrors.Is(err, context.Canceled) {
			code = http.StatusRequestTimeout
		} else if code, ok = sentinelToCode(err); !ok {
			code = http.StatusInternalServerError
		}
	}
	if !shouldSkipSentry(err) {
		sentry.GlobalClient().CaptureError(ctx, err, nil)
	}

	se.SetStatusCode(code)
	se.SetPayload(&swagmodels.Error{Message: err.Error()})
	return se

}
