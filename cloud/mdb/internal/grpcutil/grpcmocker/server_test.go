package grpcmocker_test

import (
	"context"
	"encoding/json"
	"testing"
	"time"

	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"
	"github.com/google/go-cmp/cmp"
	"github.com/jhump/protoreflect/dynamic"
	"github.com/jhump/protoreflect/dynamic/grpcdynamic"
	"github.com/jhump/protoreflect/grpcreflect"
	"github.com/stretchr/testify/require"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	reflectpb "google.golang.org/grpc/reflection/grpc_reflection_v1alpha"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker/testproto"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestRun(t *testing.T) {
	mocker, l := RunMocker(t, "testdata/testrun.json")
	ctx := context.Background()
	client, err := grpcutil.NewConn(
		ctx,
		mocker.Addr(),
		"grpcmocker_test",
		grpcutil.ClientConfig{Security: grpcutil.SecurityConfig{Insecure: true}},
		l,
	)
	require.NoError(t, err)

	Call(t, client, "grpcmocker.TestService", "GetObject", `{"id": "1"}`, `{"object": {"id": "1", "name": "foo", "number": 42}}`)
	CallError(t, client, "grpcmocker.TestService", "GetObject", `{"id": "2"}`, codes.NotFound, "object not found")
	Call(t, client, "grpcmocker.TestService", "ListObjects", `{}`, `{"objects": [{"id": "1", "name": "foo", "number": 42}]}`)

	// Custom unimplemented that is triggered as last match
	CallError(t, client, "grpcmocker.TestService", "DeleteObject", `{}`, codes.Unimplemented, "method not implemented (but mock works)")

	StopMocker(t, mocker)
}

func RunMocker(t *testing.T, cfg string) (*grpcmocker.Server, log.Logger) {
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)

	server, err := grpcmocker.Run(
		":0",
		func(s *grpc.Server) error {
			testproto.RegisterTestServiceServer(s, &testproto.UnimplementedTestServiceServer{})
			return nil
		},
		grpcmocker.WithLogger(l),
		grpcmocker.WithExpectationsFromFile(cfg),
	)
	require.NoError(t, err)
	require.NotNil(t, server)
	return server, l
}

func StopMocker(_ *testing.T, server *grpcmocker.Server) {
	_ = server.Stop(5 * time.Second)
}

func Call(t *testing.T, client *grpc.ClientConn, service, method, req, expected string) {
	resp, err := call(t, client, service, method, req)
	require.NoError(t, err)

	m := jsonpb.Marshaler{EmitDefaults: false, OrigName: true}
	stringResp, err := m.MarshalToString(resp)
	require.NoError(t, err)

	var actual map[string]interface{}
	require.NoError(t, json.Unmarshal([]byte(stringResp), &actual))

	var expectedMap map[string]interface{}
	require.NoError(t, json.Unmarshal([]byte(expected), &expectedMap))
	require.Empty(t, cmp.Diff(expectedMap, actual))
}

func CallError(t *testing.T, client *grpc.ClientConn, service, method, req string, code codes.Code, errorMsg string) {
	resp, err := call(t, client, service, method, req)
	require.Error(t, err)
	require.Nil(t, resp)

	s, ok := status.FromError(err)
	require.True(t, ok)
	require.Equal(t, code, s.Code())
	require.Equal(t, errorMsg, s.Message())
}

func call(t *testing.T, client *grpc.ClientConn, service, method, req string) (proto.Message, error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	rc := grpcreflect.NewClient(ctx, reflectpb.NewServerReflectionClient(client))
	require.NotNil(t, rc)
	sd, err := rc.ResolveService(service)
	require.NoError(t, err)
	md := sd.FindMethodByName(method)
	require.NotNil(t, md)

	s := grpcdynamic.NewStub(client)
	protoReq := dynamic.NewMessage(md.GetInputType())
	require.NotNil(t, protoReq)
	require.NoError(t, jsonpb.UnmarshalString(req, protoReq))

	return s.InvokeRpc(ctx, md, protoReq)
}
