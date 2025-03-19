//nolint:errcheck
package config

import (
	"errors"
	"fmt"
	"io/ioutil"
	"os"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"go.uber.org/zap"
)

func TestEndpointParsing(t *testing.T) {
	asrt := assert.New(t)
	fixtures := []struct {
		network string
		addr    string
		err     error
	}{
		{"tcp6", "0.0.0.0:6060", nil},
		{"tcp4", ":6060", nil},
		{"unix", "/var/run/sock.sock", nil},
		{"tcp2", "0.0.0.0:8080", errors.New("invalid network tcp2")},
		{"", "0.0.0.0:8080", errors.New("empty network")},
		{"", "", errors.New("empty network")},
		{"tcp", "", fmt.Errorf("empty address")},
	}
	for _, fixture := range fixtures {
		e := ServeEndpoint{}
		body := []byte(fmt.Sprintf("%s://%s", fixture.network, fixture.addr))
		if fixture.err == nil {
			asrt.NoError(e.UnmarshalText(body))
			asrt.Equal(fixture.network, e.Network)
			asrt.Equal(fixture.addr, e.Addr)
		} else {
			asrt.EqualError(e.UnmarshalText(body), fixture.err.Error())
		}
	}
}

func TestDecodeConfig(t *testing.T) {
	body := []byte(
		`
[DB]
Conntype = "postgres"
Connstring = "user=lantame password=1q2w3e dbname=image sslmode=require binary_parameters=yes"
Maxconns = 8
Transaction = true

[MDS]
Host = "storage-int.mdst.yandex.net"
WPort = 1111
RPort = 80
Auth = "Basic Z2xhbmNlOjA3OGVkMDE5OWI2YmI5MTE1N2VlZmRkZjc3OGRhYzc3"
Namespace = "glance"
NoKeepAlive = false
Real = false
NoUpdateReal = false

[Server]
SSL = false
CertFile = "api/snapshot/server/server/snapshot.crt"
KeyFile = "api/snapshot/server/server/snapshot.key"
HTTPEndpoint = "tcp6://0.0.0.0:8000"
GRPCEndpoint = "tcp4://127.0.0.1:9000"

[Logging]
Level = "info"
Output = "/dev/stdout"

[DebugServer]
HTTPEndpoint = "tcp4://127.0.0.1:7000"
`)
	f, err := ioutil.TempFile("", "image_temp_config")
	if err != nil {
		t.Fatalf("unable to create temp config %v", err)
	}
	defer os.Remove(f.Name())

	_, err = f.Write(body)
	require.NoError(t, err)
	require.NoError(t, f.Close())

	_ = LoadConfig(f.Name())
	asrt := assert.New(t)
	asrt.NotNil(conf)

	asrt.Equal(zap.InfoLevel, conf.Logging.Level)
	asrt.Equal("/dev/stdout", conf.Logging.Output)
	asrt.Equal(&ServeEndpoint{Network: "tcp6", Addr: "0.0.0.0:8000"}, conf.Server.HTTPEndpoint)
	asrt.Equal(&ServeEndpoint{Network: "tcp4", Addr: "127.0.0.1:9000"}, conf.Server.GRPCEndpoint)
	asrt.Equal(&ServeEndpoint{Network: "tcp4", Addr: "127.0.0.1:7000"}, conf.DebugServer.HTTPEndpoint)
}
