package main

import (
	"context"
	"crypto/tls"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"sync"

	"github.com/go-chi/chi/v5"
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/resourcemanager/v1"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker/expectations"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker/parsers"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/slices"
)

type ResponderGetPermissionStages struct {
	mu               sync.Mutex
	permissionStages map[string][]string
}

func (responder *ResponderGetPermissionStages) FuncName() string {
	return "get_permission_stages"
}

func (responder *ResponderGetPermissionStages) Respond(_ context.Context, inputs expectations.RespondInputs) (interface{}, error) {
	req := inputs.ProtoRequest.(*resourcemanager.GetPermissionStagesRequest)
	resp := &resourcemanager.GetPermissionStagesResponse{}

	responder.mu.Lock()
	defer responder.mu.Unlock()
	stages, ok := responder.permissionStages[req.CloudId]
	if ok {
		resp.PermissionStages = stages
	}

	return resp, nil
}

type AddPermissionStagesRequest struct {
	PermissionStages []string `json:"permissionStages"`
}

func DecodeJSON(r io.Reader, v interface{}) error {
	defer func() { _, _ = io.Copy(ioutil.Discard, r) }()
	return json.NewDecoder(r).Decode(v)
}

func (responder *ResponderGetPermissionStages) Register(router *chi.Mux) error {
	router.Post("/v1/clouds/{cloud_id}:addPermissionStages", func(w http.ResponseWriter, r *http.Request) {
		cloudID := chi.URLParam(r, "cloud_id")

		var req AddPermissionStagesRequest
		if err := DecodeJSON(r.Body, &req); err != nil {
			fmt.Println(err)
			_, _ = w.Write([]byte(err.Error()))
			w.WriteHeader(http.StatusBadRequest)
		}

		responder.mu.Lock()
		defer responder.mu.Unlock()
		responder.permissionStages[cloudID] = slices.DedupStrings(append(responder.permissionStages[cloudID], req.PermissionStages...))
		w.WriteHeader(http.StatusOK)
	})

	return nil
}

func main() {
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		panic(err)
	}

	cert, err := tls.LoadX509KeyPair("./cert.pem", "./key.pem")
	if err != nil {
		panic(err)
	}

	p := parsers.NewDefaultParser()
	rcParser := parsers.NewResponderCodeParser()
	stages := &ResponderGetPermissionStages{permissionStages: map[string][]string{}}
	if err = rcParser.RegisterCodeResponder(stages); err != nil {
		panic(err)
	}
	if err = p.RegisterResponderParser(rcParser); err != nil {
		panic(err)
	}

	if err = grpcmocker.RunAndWait(
		":4040",
		func(s *grpc.Server) error {
			resourcemanager.RegisterCloudServiceServer(s, &resourcemanager.UnimplementedCloudServiceServer{})
			resourcemanager.RegisterFolderServiceServer(s, &resourcemanager.UnimplementedFolderServiceServer{})
			return nil
		},
		grpcmocker.WithTLS(
			&tls.Config{
				Certificates: []tls.Certificate{cert},
				ClientAuth:   tls.NoClientCert,
			},
		),
		grpcmocker.WithExpectationsFromFile("./expectations.json"),
		grpcmocker.WithParser(p),
		grpcmocker.WithDynamicControl(stages.Register),
		grpcmocker.WithLogger(l),
	); err != nil {
		panic(err)
	}
}
