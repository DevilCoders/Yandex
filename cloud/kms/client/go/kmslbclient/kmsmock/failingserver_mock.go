package kmsmock

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"math/rand"
	"net"
	"strconv"
	"sync/atomic"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/health/grpc_health_v1"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/kms/v1"
)

const (
	ephemeralPortFrom = 50000
	ephemeralPortTo   = 65536
)

type FailingServerState struct {
	FailureRate     float64
	PingFailureRate float64
	PingTime        time.Duration

	Calls      int64
	CheckCalls int64
}

type failingServer struct {
	state *FailingServerState
}

func (m failingServer) randomPingFailure() bool {
	return rand.Float64() >= 1-m.state.PingFailureRate
}

func (m failingServer) randomFailure() bool {
	return rand.Float64() >= 1-m.state.FailureRate
}

func (m failingServer) Check(ctx context.Context, req *grpc_health_v1.HealthCheckRequest) (*grpc_health_v1.HealthCheckResponse, error) {
	if m.randomPingFailure() {
		return nil, status.Error(codes.Unavailable, "randomly failed")
	}

	if m.state.PingTime > 0 {
		time.Sleep(m.state.PingTime)
	}

	atomic.AddInt64(&m.state.CheckCalls, 1)
	ret := &grpc_health_v1.HealthCheckResponse{
		Status: grpc_health_v1.HealthCheckResponse_SERVING,
	}
	return ret, nil
}

func (m failingServer) Watch(*grpc_health_v1.HealthCheckRequest, grpc_health_v1.Health_WatchServer) error {
	return status.Error(codes.Unimplemented, "not implemented")
}

type mockCiphertext struct {
	Plaintext  []byte
	AadContext []byte
}

func (m failingServer) Encrypt(ctx context.Context, req *kms.SymmetricEncryptRequest) (*kms.SymmetricEncryptResponse, error) {
	if m.randomFailure() {
		return nil, status.Error(codes.Unavailable, "randomly failed")
	}

	atomic.AddInt64(&m.state.Calls, 1)
	ciphertext, err := json.Marshal(&mockCiphertext{
		Plaintext:  req.Plaintext,
		AadContext: req.AadContext,
	})
	if err != nil {
		return nil, err
	}
	ret := &kms.SymmetricEncryptResponse{
		KeyId:      req.KeyId,
		VersionId:  req.VersionId,
		Ciphertext: ciphertext,
	}
	return ret, nil
}

func (m failingServer) Decrypt(ctx context.Context, req *kms.SymmetricDecryptRequest) (*kms.SymmetricDecryptResponse, error) {
	if m.randomFailure() {
		return nil, status.Error(codes.Unavailable, "randomly failed")
	}

	atomic.AddInt64(&m.state.Calls, 1)
	var ciphertext mockCiphertext
	err := json.Unmarshal(req.Ciphertext, &ciphertext)
	if err != nil {
		return nil, status.Error(codes.InvalidArgument, "Wrong ciphertext: "+err.Error())
	}
	if !bytes.Equal(ciphertext.AadContext, req.AadContext) {
		return nil, status.Error(codes.InvalidArgument, "Different aad")
	}
	time.Sleep(50 * time.Millisecond)
	ret := &kms.SymmetricDecryptResponse{
		KeyId:     req.KeyId,
		VersionId: "unknown",
		Plaintext: ciphertext.Plaintext,
	}
	return ret, nil
}

func (m failingServer) BatchEncrypt(ctx context.Context, req *kms.SymmetricBatchEncryptRequest) (*kms.SymmetricBatchEncryptResponse, error) {
	return nil, status.Error(codes.Unimplemented, "not implemented")
}

func (m failingServer) BatchDecrypt(ctx context.Context, req *kms.SymmetricBatchDecryptRequest) (*kms.SymmetricBatchDecryptResponse, error) {
	return nil, status.Error(codes.Unimplemented, "not implemented")
}

func (m failingServer) ReEncrypt(ctx context.Context, req *kms.SymmetricReEncryptRequest) (*kms.SymmetricReEncryptResponse, error) {
	return nil, status.Error(codes.Unimplemented, "not implemented")
}

func (m failingServer) GenerateDataKey(ctx context.Context, req *kms.GenerateDataKeyRequest) (*kms.GenerateDataKeyResponse, error) {
	return nil, status.Error(codes.Unimplemented, "not implemented")
}

func (m failingServer) GenerateRandom(ctx context.Context, req *kms.GenerateRandomRequest) (*kms.GenerateRandomResponse, error) {
	return nil, status.Error(codes.Unimplemented, "not implemented")
}

func StartNewFailingServer(state *FailingServerState) (string, *grpc.Server, error) {
	grpcServer := grpc.NewServer()
	serv := &failingServer{state: state}
	kms.RegisterSymmetricCryptoServiceServer(grpcServer, serv)
	grpc_health_v1.RegisterHealthServer(grpcServer, serv)
	var port int
	for port = ephemeralPortFrom; port < ephemeralPortTo; port++ {
		lis, err := net.Listen("tcp", fmt.Sprintf("localhost:%d", port))
		if err == nil {
			go func() {
				_ = grpcServer.Serve(lis)
			}()
			break
		}
	}
	if port == ephemeralPortTo {
		desc := fmt.Sprintf("Could not find a port to listen on in range [%d-%d]", ephemeralPortFrom, ephemeralPortTo)
		return "", nil, errors.New(desc)
	}
	addr := "localhost:" + strconv.Itoa(port)
	return addr, grpcServer, nil
}
