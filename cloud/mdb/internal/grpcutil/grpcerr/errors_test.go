package grpcerr

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"
	"google.golang.org/grpc/codes"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestErrorToGRPC(t *testing.T) {
	inputs := []struct {
		err        error
		expose     bool
		outCode    codes.Code
		outMessage string
		outDetails string
	}{
		{
			err:        xerrors.New("foo"),
			outCode:    codes.Unknown,
			outMessage: "Unknown error",
			outDetails: "[]",
		},
		{
			err:        xerrors.New("foo"),
			expose:     true,
			outCode:    codes.Unknown,
			outMessage: "foo",
			outDetails: "[detail:\"foo\\n    a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr.TestErrorToGRPC\\n        cloud/mdb/internal/grpcutil/grpcerr/errors_test.go:30\\n\"]",
		},
		{
			err:        semerr.Authentication("bar"),
			outCode:    codes.Unauthenticated,
			outMessage: "bar",
			outDetails: "[]",
		},
		{
			err:        semerr.Authentication("bar"),
			expose:     true,
			outCode:    codes.Unauthenticated,
			outMessage: "bar",
			outDetails: "[detail:\"bar\\n    a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr.TestErrorToGRPC\\n        cloud/mdb/internal/grpcutil/grpcerr/errors_test.go:43\\n\"]",
		},
		{
			err:        xerrors.Errorf("this happened: %w", semerr.Unavailable("foobar")),
			outCode:    codes.Unavailable,
			outMessage: "foobar",
			outDetails: "[]",
		},
		{
			err:        xerrors.Errorf("this happened: %w", semerr.Unavailable("foobar")),
			expose:     true,
			outCode:    codes.Unavailable,
			outMessage: "foobar",
			outDetails: "[detail:\"this happened:\\n    a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr.TestErrorToGRPC\\n        cloud/mdb/internal/grpcutil/grpcerr/errors_test.go:56\\nfoobar\\n    a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr.TestErrorToGRPC\\n        cloud/mdb/internal/grpcutil/grpcerr/errors_test.go:56\\n\"]",
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprintf("%s/%t", input.err, input.expose), func(t *testing.T) {
			st := ErrorToGRPC(input.err, input.expose, &nop.Logger{})
			assert.Equal(t, input.outCode, st.Code())
			assert.Equal(t, input.outMessage, st.Message())
			assert.Equal(t, input.outDetails, fmt.Sprintf("%+v", st.Details()))
		})
	}
}
