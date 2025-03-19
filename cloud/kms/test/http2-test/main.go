package main

import (
	"context"
	"crypto/tls"
	"flag"
	"golang.org/x/net/http2"
	"io"
	"log"
	"net/http"
	"net/http/httptrace"
	"time"
)

const (
	readIdleTimeout = time.Second * 60
	reqTimeout      = time.Second * 30
)

var (
	downloadURL     = flag.String("url", "", "repeatedly download URL")
	disableHTTP2    = flag.Bool("disable-http2", false, "disable HTTP support")
	enableHTTP2Idle = flag.Bool("enable-http2-idle", false, "enable ReadIdleTimeout for HTTP2")
	exitOnError     = flag.Bool("exit-on-error", false, "exit on first error")
)

func main() {
	flag.Parse()

	if *disableHTTP2 {
		log.Printf("Disabling HTTP2\n")
		// See https://github.com/golang/go/issues/39302
		defaultTransport := http.DefaultTransport.(*http.Transport)
		defaultTransport.ForceAttemptHTTP2 = false
		defaultTransport.TLSClientConfig = &tls.Config{}
		defaultTransport.TLSNextProto = make(map[string]func(authority string, c *tls.Conn) http.RoundTripper)
	} else {
		if *enableHTTP2Idle {
			log.Printf("Setting HTTP2 ReadIdleTimeout\n")
			h2Transport, err := http2.ConfigureTransports(http.DefaultTransport.(*http.Transport))
			if err != nil {
				log.Fatalf("could not configure HTTP transports: %v\n", err)
			}
			h2Transport.ReadIdleTimeout = readIdleTimeout
		}
	}
	client := &http.Client{Timeout: reqTimeout}

	for {
		log.Printf("Downloading from %s", *downloadURL)

		success := func() bool {
			startTime := time.Now()
			log.Printf("Downloading...\n")
			ctx, cancel := context.WithTimeout(context.Background(), reqTimeout)
			ctx = httptrace.WithClientTrace(ctx, &httptrace.ClientTrace{
				GetConn: func(hostPort string) {
					log.Printf("HTTP: GetConn to %s\n", hostPort)
				},
				GotConn: func(info httptrace.GotConnInfo) {
					log.Printf("HTTP: GotConn %v\n", info)
				},
				PutIdleConn: func(err error) {
					log.Printf("HTTP: PutIdleConn: %v\n", err)
				},
				ConnectStart: func(network, addr string) {
					log.Printf("HTTP: ConnectStart %s (%s)\n", addr, network)
				},
				ConnectDone: func(network, addr string, err error) {
					log.Printf("HTTP: ConnectDone %s (%s): %v\n", addr, network, err)
				},
			})
			defer cancel()
			req, err := http.NewRequestWithContext(ctx, http.MethodGet, *downloadURL, nil)
			if err != nil {
				log.Printf("ERROR: new request failed: %v\n", err)
				return false
			}
			resp, err := client.Do(req)
			if err != nil {
				log.Printf("ERROR: downloading failed: %v\n", err)
				return false
			}
			log.Printf("Got response: %v\n", resp)

			data, err := io.ReadAll(resp.Body)
			if err != nil {
				log.Printf("ERROR: reading failed, read %d: %v\n", len(data), err)
				_ = resp.Body.Close()
				return false
			}
			err = resp.Body.Close()
			if err != nil {
				log.Printf("ERROR: closing failed: %v\n", err)
			}
			log.Printf("Downloaded in %v\n", time.Since(startTime))
			return true
		}()

		if !success && *exitOnError {
			return
		}
	}
}
