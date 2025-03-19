package grpc

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"
	"google.golang.org/grpc/codes"
	grpcstatus "google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	mlockapi "a.yandex-team.ru/cloud/mdb/mlock/api"
	mlockmock "a.yandex-team.ru/cloud/mdb/mlock/api/mocks"
)

type notFoundGRPCErr struct {
}

func (e notFoundGRPCErr) GRPCStatus() *grpcstatus.Status {
	return grpcstatus.New(codes.NotFound, "")
}

func (e notFoundGRPCErr) Error() string {
	return ""
}

var _ error = &notFoundGRPCErr{}

func TestLockerAcquireLock(t *testing.T) {
	const holderName = "test-holder"
	type input struct {
		id      string
		holder  string
		fqdns   []string
		reason  string
		prepare func(db *mlockmock.MockLockServiceClient)
	}
	type output struct {
	}
	type testCase struct {
		name   string
		input  input
		expect output
	}
	tcs := []testCase{
		{
			name: "happy path",
			input: input{
				id:     "lock-id",
				holder: holderName,
				fqdns:  []string{"test-object"},
				reason: "test",
				prepare: func(mlock *mlockmock.MockLockServiceClient) {
					mlock.EXPECT().CreateLock(
						gomock.Any(),
						&mlockapi.CreateLockRequest{
							Id:      "lock-id",
							Holder:  holderName,
							Reason:  "test",
							Objects: []string{"test-object"},
						},
					).Return(
						&mlockapi.CreateLockResponse{}, nil,
					)

				},
			},
			expect: output{},
		},
	}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			mlock := mlockmock.NewMockLockServiceClient(gomock.NewController(t))
			ctx := context.Background()
			tc.input.prepare(mlock)
			locker := NewMlockGRPCClient(mlock)
			err := locker.CreateLock(ctx, tc.input.id, tc.input.holder, tc.input.fqdns, tc.input.reason)
			require.NoError(t, err)
		})
	}
}

func TestLockerReleaseLock(t *testing.T) {
	const holderName = "test-holder"
	type input struct {
		lockID  string
		prepare func(db *mlockmock.MockLockServiceClient)
	}
	type output struct {
		withErr func(err error) bool
	}
	type testCase struct {
		name   string
		input  input
		expect output
	}
	tcs := []testCase{
		{
			name: "happy path",
			input: input{
				lockID: "lock-id",
				prepare: func(mlock *mlockmock.MockLockServiceClient) {
					mlock.EXPECT().ReleaseLock(
						gomock.Any(),
						gomock.Any(),
					).Return(
						&mlockapi.ReleaseLockResponse{}, nil,
					)

				},
			},
			expect: output{},
		},
		{
			name: "not found",
			input: input{
				lockID: "lock-id",
				prepare: func(mlock *mlockmock.MockLockServiceClient) {

					mlock.EXPECT().ReleaseLock(
						gomock.Any(),
						gomock.Any(),
					).Return(
						nil, &notFoundGRPCErr{},
					)

				},
			},
			expect: output{
				withErr: func(err error) bool {
					return semerr.IsNotFound(err)
				},
			},
		},
	}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			mlock := mlockmock.NewMockLockServiceClient(gomock.NewController(t))
			ctx := context.Background()
			tc.input.prepare(mlock)
			locker := NewMlockGRPCClient(mlock)
			err := locker.ReleaseLock(ctx, tc.input.lockID)
			if tc.expect.withErr != nil {
				require.True(t, tc.expect.withErr(err))
			} else {
				require.NoError(t, err)
			}

		})
	}
}
