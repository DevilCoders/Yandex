package kmslbclient

import (
	"context"
	"log"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/kms/client/go/kmslbclient/kmsmock"
)

func TestAll(t *testing.T) {
	state1 := &kmsmock.FailingServerState{
		FailureRate:     1.0,
		PingFailureRate: 1.0,
		PingTime:        10 * time.Millisecond,
	}
	addr1, grpcServer1, err := kmsmock.StartNewFailingServer(state1)
	require.NoErrorf(t, err, "Error when starting new failing server: %v", err)
	defer grpcServer1.Stop()
	state2 := &kmsmock.FailingServerState{
		FailureRate:     0.0,
		PingFailureRate: 0.0,
		PingTime:        0,
	}
	addr2, grpcServer2, err := kmsmock.StartNewFailingServer(state2)
	require.NoErrorf(t, err, "Error when starting new failing server: %v", err)
	defer grpcServer2.Stop()
	state3 := &kmsmock.FailingServerState{
		FailureRate:     0.01,
		PingFailureRate: 0.0,
		PingTime:        0,
	}
	addr3, grpcServer3, err := kmsmock.StartNewFailingServer(state3)
	require.NoErrorf(t, err, "Error when starting new failing server: %v", err)
	defer grpcServer3.Stop()
	state4 := &kmsmock.FailingServerState{
		FailureRate:     0.0,
		PingFailureRate: 0.0,
		PingTime:        1000 * time.Millisecond,
	}
	addr4, grpcServer4, err := kmsmock.StartNewFailingServer(state4)
	require.NoErrorf(t, err, "Error when starting new failing server: %v", err)
	defer grpcServer4.Stop()

	targets := []string{addr1, addr2, addr3, addr4}
	log.Printf("Serving on %v", targets)
	clientAddr := &ClientAddress{TargetAddrs: targets}
	options := &ClientOptions{
		TokenProvider: func() string { return "fakeToken" },
		// Logger:        NewLoggerFromLog(log.New(log.Writer(), "KMS", 0), DebugPriority),
		Plaintext: true,
		Balancer: BalancerOptions{
			ConnectionsPerHost:   1,
			KeepAliveTime:        time.Second,
			PingRefreshTime:      time.Second,
			MaxPing:              100 * time.Second,
			NumPings:             1,
			SimilarPingThreshold: 100 * time.Millisecond,
			MaxTimePerTry:        time.Second,
			MaxFailCountPerHost:  10,
			FailWindowPerHost:    time.Second,
		},
	}
	client, err := NewLBSymmetricCryptoClient(clientAddr, options)
	require.NoErrorf(t, err, "Error when starting new failing server: %v", err)
	defer client.Close()

	const numCalls = 100
	for i := 0; i < numCalls; i++ {
		_, err := client.Encrypt(context.Background(), "keyId", []byte("plaintext"), nil, nil)
		require.NoErrorf(t, err, "Call failed")
	}

	assert.EqualValues(t, state1.Calls, 0)
	assert.EqualValues(t, state2.Calls+state3.Calls, numCalls)
	assert.EqualValues(t, state4.Calls, 0)
	if state2.Calls == 0 || state3.Calls == 0 {
		t.Errorf("Unbalanced calls to two live backends: %v vs %v", state2.Calls, state3.Calls)
	}
}
