package server

import (
	"fmt"
	"net"
	"net/http"
)

const (
	fileName = "cirros-0.3.5-x86_64-disk.img"
	rawImage = "random-image.raw"
)

type TestingServer struct {
	*http.Server
}

func NewServer() *TestingServer {
	return &TestingServer{}
}

func (srv *TestingServer) Serve(listener net.Listener) error {
	multiplexer := http.NewServeMux()
	multiplexer.HandleFunc("/file", func(writer http.ResponseWriter, request *http.Request) {
		http.ServeFile(writer, request, fileName)
	})
	multiplexer.HandleFunc("/raw-image", func(writer http.ResponseWriter, request *http.Request) {
		http.ServeFile(writer, request, rawImage)
	})
	multiplexer.HandleFunc("/redirect", func(writer http.ResponseWriter, request *http.Request) {
		http.Redirect(writer, request, FileURL(listener), http.StatusSeeOther)
	})
	srv.Server = &http.Server{Handler: multiplexer}
	return srv.Server.Serve(listener)
}

func FileURL(l net.Listener) string {
	return fmt.Sprintf("http://%s/file", l.Addr().String())
}

func RawFileURL(l net.Listener) string {
	return fmt.Sprintf("http://%s/raw-image", l.Addr().String())
}

func RedirectURL(l net.Listener) string {
	return fmt.Sprintf("http://%s/redirect", l.Addr().String())
}
