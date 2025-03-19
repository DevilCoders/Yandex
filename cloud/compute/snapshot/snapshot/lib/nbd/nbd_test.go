package nbd

import (
	"context"
	"fmt"
	"io/ioutil"
	"net"
	"net/http"
	"net/http/httptest"
	"net/http/httputil"
	"net/url"
	"os"
	"path/filepath"
	"testing"

	"cuelang.org/go/pkg/crypto/md5"

	"go.uber.org/zap"

	"go.uber.org/zap/zaptest"

	"github.com/stretchr/testify/require"

	"cuelang.org/go/pkg/time"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"

	gcontext "golang.org/x/net/context"

	"github.com/stretchr/testify/assert"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
)

const (
	convertURL = "http://127.0.0.1:1234/cirros-0.3.5-x86_64-disk.img"
)

type testImg struct {
	blockDev string
}

func (t testImg) GetQemuBlockdev(ctx gcontext.Context) (string, error) {
	return t.blockDev, nil
}

func (t testImg) GetChunkSize() int {
	return storage.DefaultChunkSize
}

func (t testImg) GetReaderCount(maxCount int) int {
	return 1
}

func (t testImg) GetFormat() string {
	return ""
}

func Test_NbdRegistryMountNBD(t *testing.T) {
	at := assert.New(t)
	ctx := log.WithLogger(context.Background(), zaptest.NewLogger(t, zaptest.WrapOptions(zap.Development())))
	ctx, ctxCancel := context.WithTimeout(ctx, time.Second*30)
	defer ctxCancel()

	dir, err := ioutil.TempDir("/tmp", "snapshot-nbd-test-")
	if err != nil {
		t.Fatal(err)
	}

	defer os.RemoveAll(dir)

	httpServer := httptest.NewServer(http.FileServer(http.Dir("")))
	defer httpServer.Close()

	d, err := os.Open(".")
	at.NoError(err)
	names, err := d.Readdirnames(-1)
	at.NoError(err)
	log.G(ctx).Info("dir content", zap.Strings("names", names))

	proxyPath := filepath.Join(dir, "proxy.sock")
	err = createProxy(ctx, proxyPath, httpServer.URL)
	if err != nil {
		t.Fatal(err)
	}

	n := func() *nbdRegistry {
		r := &nbdRegistry{
			dir:             dir,
			proxySocketPath: proxyPath,
		}
		err := r.init(ctx)
		if err != nil {
			t.Fatal(err)
		}
		return r
	}

	img := testImg{blockDev: convertURL}
	r := n()
	nbd, err := r.MountNBD(ctx, img)
	at.NoError(err)
	require.NotNil(t, nbd)
	at.True(nbd.Size() > 0)

	block := make([]byte, storage.DefaultChunkSize*2)
	bytesReaded, err := nbd.ReadAt(block[:storage.DefaultChunkSize], 0)
	at.NoError(err)
	at.Equal(storage.DefaultChunkSize, bytesReaded)
	bytesReaded, err = nbd.ReadAt(block[storage.DefaultChunkSize:], storage.DefaultChunkSize)
	at.NoError(err)
	at.Equal(storage.DefaultChunkSize, bytesReaded)
	s := fmt.Sprintf("%x", md5.Sum(block))
	at.Equal("4fb872bbf8238a008e5382c8cea740d9", s)
	err = nbd.Close(ctx)
	at.NoError(err)
}

func createProxy(ctx context.Context, socketPath, targetURL string) error {
	listener, err := net.Listen("unix", socketPath)
	if err != nil {
		return err
	}

	u, err := url.Parse(targetURL)
	if err != nil {
		return err
	}
	proxy := httputil.NewSingleHostReverseProxy(u)

	httpServer := http.Server{Handler: proxy}
	go func() {
		_ = httpServer.Serve(listener)
	}()
	go func() {
		<-ctx.Done()
		_ = httpServer.Close()
	}()
	return nil
}
