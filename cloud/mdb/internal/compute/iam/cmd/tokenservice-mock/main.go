package main

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"net/http"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"
	"google.golang.org/grpc"
	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1"
	ts "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1/ts"
	"a.yandex-team.ru/library/go/httputil/render"
)

const grpcPort = ":50051"
const restPort = ":50052"

func main() {
	lis, err := net.Listen("tcp", grpcPort)
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}
	go restServer(restPort)
	log.Printf("grpc listening on %s", grpcPort)
	s := grpc.NewServer()
	iam.RegisterIamTokenServiceServer(s, &tokenService{})
	if err := s.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
	log.Printf("finished serving")
}

type tokenService struct {
	iam.UnimplementedIamTokenServiceServer
}

func (i *tokenService) Create(ctx context.Context, req *iam.CreateIamTokenRequest) (*iam.CreateIamTokenResponse, error) {
	log.Printf("returning dummy token for identity %+v", req.Identity)
	return tokenResponse(), nil
}

func restServer(address string) {
	r := chi.NewRouter()

	r.Use(middleware.RequestID)
	r.Use(middleware.Logger)
	r.Use(middleware.Recoverer)
	r.Use(middleware.URLFormat)

	r.Get("/ping", func(w http.ResponseWriter, r *http.Request) {
		_, _ = w.Write([]byte("pong"))
	})

	r.Post("/iam/v1/tokens", CreateToken) // POST /iam/v1/tokens
	log.Printf("rest listening on %s", address)
	if err := http.ListenAndServe(address, r); err != nil {
		log.Printf("listenning rest on %q: %s", address, err.Error())
	}
}

func CreateToken(w http.ResponseWriter, r *http.Request) {
	data := &struct {
		OauthToken string `json:"oauthToken"`
		JWT        string `json:"jwt"`
	}{}
	defer r.Body.Close()
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		w.WriteHeader(http.StatusBadRequest)
		_, _ = w.Write([]byte(fmt.Sprintf("read body: %s", err.Error())))
		return
	}
	err = json.Unmarshal(body, data)
	if err != nil {
		w.WriteHeader(http.StatusBadRequest)
		_, _ = w.Write([]byte(fmt.Sprintf("unmarshal body: %s", err.Error())))
		return
	}
	_, err = render.JSON(w, &struct {
		IamToken  string    `json:"iamToken"`
		ExpiresAt time.Time `json:"expiresAt"`
	}{
		IamToken:  "t.1dummy_token",
		ExpiresAt: time.Now().Add(time.Hour * 24),
	})
	if err != nil {
		w.WriteHeader(http.StatusBadRequest)
		_, _ = w.Write([]byte(fmt.Sprintf("marshalling body: %s", err.Error())))
	}
}

func tokenResponse() *iam.CreateIamTokenResponse {
	return &iam.CreateIamTokenResponse{
		IamToken:  "t.1dummy_token",
		IssuedAt:  timestamppb.Now(),
		ExpiresAt: timestamppb.New(time.Now().Add(time.Hour * 24)),
		Subject: &ts.Subject{Type: &ts.Subject_ServiceAccount_{
			ServiceAccount: &ts.Subject_ServiceAccount{Id: "test_service_account_id"},
		}},
	}
}
