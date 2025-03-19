package main

import (
	"context"
	"errors"
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"net/http"
	"os"
	"strings"
	"sync"

	"github.com/golang/protobuf/proto"
	"github.com/grpc-ecosystem/grpc-gateway/runtime"
	"github.com/spf13/cobra"
	"google.golang.org/grpc"

	filestore_config "a.yandex-team.ru/cloud/filestore/config"
	filestore_grpc "a.yandex-team.ru/cloud/filestore/public/api/grpc"
)

////////////////////////////////////////////////////////////////////////////////

func parseConfig(
	configFileName string,
	config *filestore_config.THttpProxyConfig,
) error {

	configBytes, err := ioutil.ReadFile(configFileName)
	if err != nil {
		return fmt.Errorf(
			"failed to read config file %v: %w",
			configFileName,
			err,
		)
	}

	err = proto.UnmarshalText(string(configBytes), config)
	if err != nil {
		return fmt.Errorf(
			"failed to parse config file %v as protobuf: %w",
			configFileName,
			err,
		)
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func createEndpoint(
	nfsVhostHost string,
	nfsVhostPort uint32,
) (string, error) {

	if nfsVhostHost == "localhost-expanded" {
		hostname, err := os.Hostname()
		if err != nil {
			return "", err
		}

		return fmt.Sprintf("%v:%v", hostname, nfsVhostPort), nil
	}

	return fmt.Sprintf("%v:%v", nfsVhostHost, nfsVhostPort), nil
}

////////////////////////////////////////////////////////////////////////////////

func isNfsHeaderKey(key string) (string, bool) {
	key = strings.ToLower(key)
	if strings.HasPrefix(key, "x-nfs-") {
		return key, true
	}
	return key, false
}

////////////////////////////////////////////////////////////////////////////////

func createProxyMux(
	ctx context.Context,
	config *filestore_config.THttpProxyConfig,
) (*runtime.ServeMux, error) {

	endpoint, err := createEndpoint(*config.NfsVhostHost, *config.NfsVhostPort)
	if err != nil {
		return nil, fmt.Errorf("failed to create endpoint: %w", err)
	}
	log.Printf("Forwarding to (insecure) %v", endpoint)

	mux := runtime.NewServeMux(
		runtime.WithIncomingHeaderMatcher(func(key string) (string, bool) {
			if key, ok := isNfsHeaderKey(key); ok {
				return key, true
			}
			return runtime.DefaultHeaderMatcher(key)
		}),
	)
	transportDialOption := grpc.WithInsecure()
	err = filestore_grpc.RegisterTEndpointManagerServiceHandlerFromEndpoint(
		ctx,
		mux,
		endpoint,
		[]grpc.DialOption{
			transportDialOption,
		},
	)
	if err != nil {
		return nil, fmt.Errorf("failed to register TEndpointManagerService handler: %w", err)
	}

	return mux, nil
}

type httpProxyServer struct {
	addr string
	wait func()
	kill func()
}

func runInsecureServer(
	mux *runtime.ServeMux,
	config *filestore_config.THttpProxyConfig,
) (*httpProxyServer, error) {

	addr := fmt.Sprintf(":%v", *config.Port)

	lis, err := net.Listen("tcp", addr)
	if err != nil {
		return nil, fmt.Errorf("failed to listen on address %v: %w", addr, err)
	}
	addr = lis.Addr().String()
	log.Printf("Listening on (insecure) %v", addr)

	srv := &http.Server{
		Handler: mux,
	}

	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		defer wg.Done()

		err := srv.Serve(lis)
		if err != nil && !errors.Is(err, http.ErrServerClosed) {
			log.Fatalf("Error serving HTTP: %v", err)
		}
	}()

	return &httpProxyServer{
		addr: addr,
		wait: wg.Wait,
		kill: func() {
			err := srv.Shutdown(context.Background())
			if err != nil {
				log.Fatalf("Error shutting down HTTP: %v", err)
			}
		},
	}, nil
}

func createAndRunServers(
	ctx context.Context,
	config *filestore_config.THttpProxyConfig,
) (func(), error) {

	mux, err := createProxyMux(ctx, config)
	if err != nil {
		return nil, err
	}

	var insecureServer *httpProxyServer
	if config.Port != nil {
		insecureServer, err = runInsecureServer(mux, config)
		if err != nil {
			return nil, err
		}
	}

	return func() {
		if insecureServer != nil {
			insecureServer.wait()
		}
	}, nil
}

////////////////////////////////////////////////////////////////////////////////

func main() {
	log.Println("filestore-http-proxy launched")

	var configFileName string
	config := &filestore_config.THttpProxyConfig{}

	var rootCmd = &cobra.Command{
		Use:   "filestore-http-proxy",
		Short: "HTTP proxy for NFS server",
		PersistentPreRunE: func(cmd *cobra.Command, args []string) error {
			return parseConfig(configFileName, config)
		},
		Run: func(cmd *cobra.Command, args []string) {
			ctx, cancelCtx := context.WithCancel(context.Background())
			defer cancelCtx()

			wait, err := createAndRunServers(ctx, config)
			if err != nil {
				log.Fatalf("Error: %v", err)
			}
			wait()
		},
	}
	rootCmd.Flags().StringVar(
		&configFileName,
		"config",
		"/Berkanavt/nfs-server/cfg/nfs-http-proxy.txt",
		"Path to the config file",
	)

	if err := rootCmd.Execute(); err != nil {
		log.Fatalf("Error: %v", err)
	}
}
