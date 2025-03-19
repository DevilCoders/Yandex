package interceptors

import (
	"context"
	"reflect"
	"testing"

	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log/nop"
)

// returns a combo-handler which calls handlers one after another in order they are passed
func chainHandlers(t *testing.T, handlers ...grpc.UnaryHandler) grpc.UnaryHandler {
	i := 0
	return func(ctx context.Context, req interface{}) (interface{}, error) {
		if i >= len(handlers) {
			t.Errorf("no more handlers, already called %d times", i)
			t.Fail()
		}
		res, err := handlers[i](ctx, req)
		i++
		return res, err
	}
}

func Test_unaryServerInterceptorRetry(t *testing.T) {
	type args struct {
		req     interface{}
		info    *grpc.UnaryServerInfo
		handler grpc.UnaryHandler
		backoff *retry.BackOff
	}
	tests := []struct {
		name    string
		args    args
		want    interface{}
		wantErr bool
	}{
		{
			name: "retriable, happy path",
			args: args{
				req: nil,
				handler: chainHandlers(t,
					func(context.Context, interface{}) (interface{}, error) {
						return nil, semerr.Unavailable("chtoto poshlo ne tak")
					},
					func(context.Context, interface{}) (interface{}, error) {
						return "vse_zbs", nil
					}),
				backoff: retry.New(retry.Config{
					MaxRetries: 1,
				}),
			},
			want:    "vse_zbs",
			wantErr: false,
		},
		{
			name: "retriable, max retries",
			args: args{
				req: nil,
				handler: chainHandlers(t,
					func(context.Context, interface{}) (interface{}, error) {
						return nil, semerr.Unavailable("error1")
					},
					func(context.Context, interface{}) (interface{}, error) {
						return nil, semerr.Unavailable("error2")
					}),
				backoff: retry.New(retry.Config{
					MaxRetries: 1,
				}),
			},
			want:    nil,
			wantErr: true,
		},
		{
			name: "not retriable",
			args: args{
				req: nil,
				handler: chainHandlers(t,
					func(context.Context, interface{}) (interface{}, error) {
						return nil, semerr.NotImplemented("forgot to write this method")
					}),
				backoff: retry.New(retry.Config{
					MaxRetries: 1,
				}),
			},
			want:    nil,
			wantErr: true,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, err := newUnaryServerInterceptorRetry(tt.args.backoff, &nop.Logger{})(context.Background(), tt.args.req, tt.args.info, tt.args.handler)
			if (err != nil) != tt.wantErr {
				t.Errorf("unaryServerInterceptorRetry() error = %v, wantErr %v", err, tt.wantErr)
				return
			}
			if !reflect.DeepEqual(got, tt.want) {
				t.Errorf("unaryServerInterceptorRetry() got = %v, want %v", got, tt.want)
			}
		})
	}
}
