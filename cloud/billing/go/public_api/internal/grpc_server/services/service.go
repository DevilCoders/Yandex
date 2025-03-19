package services

import (
	"context"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
	billing_grpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1"
)

var _ billing_grpc.ServiceServiceServer = &ServiceService{}

type ServiceService struct {
	client console.Client
}

func NewServiceService(client console.Client) *ServiceService {
	return &ServiceService{client: client}
}

func (s *ServiceService) Get(ctx context.Context, request *billing_grpc.GetServiceRequest) (*billing_grpc.Service, error) {

	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	consoleReq := &console.GetServiceRequest{
		AuthData: console.AuthData{Token: token},
		ID:       request.Id,
	}
	consoleService, err := s.client.GetService(ctx, consoleReq)
	if err != nil {
		return nil, parseError(err)
	}

	service := &billing_grpc.Service{
		Id:   consoleService.ID,
		Name: consoleService.Description,
	}

	return service, nil
}

func (s ServiceService) List(ctx context.Context, request *billing_grpc.ListServicesRequest) (*billing_grpc.ListServicesResponse, error) {
	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	consoleReq := &console.ListServiceRequest{
		AuthData: console.AuthData{Token: token},
		Filter:   request.Filter,
	}

	consoleResponse, err := s.client.ListServices(ctx, consoleReq)
	if err != nil {
		return nil, parseError(err)
	}

	return mapGRPCListServicesResponse(consoleResponse), nil
}

func mapGRPCListServicesResponse(consoleResponse *console.ListServiceResponse) *billing_grpc.ListServicesResponse {
	var services []*billing_grpc.Service
	for _, consoleService := range consoleResponse.Services {
		services = append(services, &billing_grpc.Service{
			Id:   consoleService.ID,
			Name: consoleService.Description, // TODO (goncharov-art): should be on python side
		})
	}

	return &billing_grpc.ListServicesResponse{Services: services}
}
