package auth

import (
	"context"
	"fmt"
	"reflect"

	"google.golang.org/grpc"

	cloudauth "a.yandex-team.ru/cloud/iam/accessservice/client/iam-access-service-client-go/v1"
)

type AuthenticationService interface {
	Authorize(ctx context.Context, token interface{}, permission Permission, resource AuthorizeResource) (*Response, error)
}

type Response struct {
	UserID   string
	FolderID string
}

type CloudIAMAccessService struct {
	client cloudauth.AccessServiceClient
}

func NewCloudIAMAccessService(conn *grpc.ClientConn) *CloudIAMAccessService {
	client := cloudauth.NewAccessServiceClient(conn)

	return &CloudIAMAccessService{
		client: client,
	}
}

func (c *CloudIAMAccessService) Authorize(ctx context.Context, token interface{}, permission Permission, resource AuthorizeResource) (*Response, error) {
	iamToken, ok := token.(cloudauth.IAMToken)
	if !ok {
		return nil, fmt.Errorf("invalid token type")
	}

	cloudResources := extractCloudResources(resource)

	clientResponse, err := c.client.Authorize(ctx, iamToken, permission.String(), cloudResources...)
	if err != nil {
		return nil, fmt.Errorf("access service authorize: %w", err)
	}

	response, err := convertToResponse(clientResponse)
	if err != nil {
		return nil, fmt.Errorf("convert client response: %w", err)
	}

	return response, nil
}

func extractCloudResources(resource AuthorizeResource) []cloudauth.Resource {
	cloudAuthResource, ok := resource.(CloudAuthResource)
	if !ok {
		return nil
	}

	return cloudAuthResource.GetResources()
}

func convertToResponse(subject cloudauth.Subject) (*Response, error) {
	response := &Response{
		UserID:   "",
		FolderID: "",
	}

	switch st := subject.(type) {
	case cloudauth.AnonymousAccount:
	case cloudauth.ServiceAccount:
		response.UserID = st.ID
		response.FolderID = st.FolderID
	case cloudauth.UserAccount:
		response.UserID = st.ID
	default:
		typeOf := reflect.TypeOf(st)
		return nil, fmt.Errorf("unexpected auth subject type: %v", typeOf)
	}

	return response, nil
}
