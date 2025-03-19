package interceptors

import (
	"context"
	"net"
	"reflect"
	"testing"

	"github.com/golang/mock/gomock"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/peer"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	sentrymocks "a.yandex-team.ru/cloud/mdb/internal/sentry/mocks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func Test_unaryServerInterceptorSentry(t *testing.T) {
	type args struct {
		ctx             context.Context
		req             interface{}
		info            *grpc.UnaryServerInfo
		handler         grpc.UnaryHandler
		setExpectations func(c *sentrymocks.MockClient)
	}
	tests := []struct {
		name    string
		args    args
		want    interface{}
		wantErr bool
	}{
		{
			name: "no errors",
			args: args{
				ctx: context.Background(),
				req: nil,
				info: &grpc.UnaryServerInfo{
					FullMethod: "method_name",
				},
				handler: func(ctx context.Context, req interface{}) (i interface{}, err error) {
					return "resp", nil
				},
			},
			want:    "resp",
			wantErr: false,
		},
		{
			name: "some error, happy path",
			args: args{
				ctx: peer.NewContext(
					requestid.WithRequestID(context.Background(), "id"),
					&peer.Peer{Addr: &net.IPAddr{}},
				),
				req: nil,
				info: &grpc.UnaryServerInfo{
					FullMethod: "method_name",
				},
				handler: func(ctx context.Context, req interface{}) (i interface{}, err error) {
					return "resp", xerrors.New("not semerr")
				},
				setExpectations: func(c *sentrymocks.MockClient) {
					c.EXPECT().CaptureError(gomock.Any(), gomock.Any(), map[string]string{
						"method":      "method_name",
						"client_addr": (&net.IPAddr{}).String(),
					})
				},
			},
			want:    "resp",
			wantErr: true,
		},
		{
			name: "unknown semerr",
			args: args{
				ctx:  context.Background(),
				req:  nil,
				info: &grpc.UnaryServerInfo{},
				handler: func(ctx context.Context, req interface{}) (i interface{}, err error) {
					// Only way to create semerr.Unknown via public functions is to pass semerr through whitelist
					return "resp", semerr.WhitelistErrors(semerr.Authentication("hz, cheto"), semerr.SemanticNotFound)
				},
				setExpectations: func(c *sentrymocks.MockClient) {
					c.EXPECT().CaptureError(gomock.Any(), gomock.Any(), gomock.Any())
				},
			},
			want:    "resp",
			wantErr: true,
		},
		{
			name: "known semerr",
			args: args{
				ctx:  context.Background(),
				req:  nil,
				info: &grpc.UnaryServerInfo{},
				handler: func(ctx context.Context, req interface{}) (i interface{}, err error) {
					return "resp", semerr.Authentication("ya tebya ne znayu")
				},
			},
			want:    "resp",
			wantErr: true,
		},
		{
			name: "known status",
			args: args{
				ctx:  context.Background(),
				req:  nil,
				info: &grpc.UnaryServerInfo{},
				handler: func(ctx context.Context, req interface{}) (i interface{}, err error) {
					return "resp", status.Error(codes.PermissionDenied, "ne polozheno")
				},
			},
			want:    "resp",
			wantErr: true,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			c := sentrymocks.NewMockClient(ctrl)
			sentry.SetGlobalClient(c)
			if tt.args.setExpectations != nil {
				tt.args.setExpectations(c)
			}
			inter := newUnaryServerInterceptorSentry()

			got, err := inter(tt.args.ctx, tt.args.req, tt.args.info, tt.args.handler)
			if (err != nil) != tt.wantErr {
				t.Errorf("unaryServerInterceptorSentry() error = %v, wantErr %v", err, tt.wantErr)
				return
			}
			if !reflect.DeepEqual(got, tt.want) {
				t.Errorf("unaryServerInterceptorSentry() got = %v, want %v", got, tt.want)
			}
		})
	}
}
