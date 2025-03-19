package sentry

import (
	"context"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func NeedReport(err error) bool {
	// Do not report cancellations (canceled by 'user') or exceeded deadlines (sentry's job is to report hard errors, not timeouts that happen in the system)
	if xerrors.Is(err, context.Canceled) || xerrors.Is(err, context.DeadlineExceeded) {
		return false
	}

	sem := semerr.AsSemanticError(err)
	isSemantic := sem != nil
	semerrUnknown := isSemantic && sem.Semantic == semerr.SemanticUnknown
	grpcUnknown := !isSemantic && status.Code(err) == codes.Unknown
	return semerrUnknown || grpcUnknown
}
