package auth

import "context"

type MockAuthService struct{}

func (s *MockAuthService) Authorize(ctx context.Context, token interface{}, permission Permission, resource AuthorizeResource) (*Response, error) {
	return &Response{
		UserID:   "mock_user_id",
		FolderID: "mock_folder_id",
	}, nil
}
