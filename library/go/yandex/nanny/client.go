package nanny

import "context"

type ListOptions struct {
	ExcludeRuntimeAttrs bool
	Category            string
	Limit, Skip         int
}

type Client interface {
	ListServices(ctx context.Context, opts ListOptions) ([]Service, error)
	GetService(ctx context.Context, serviceID string) (*Service, error)
	CreateService(ctx context.Context, service *ServiceSpec, comment string) (*Service, error)
	DeleteService(ctx context.Context, serviceID string) error

	GetInfoAttrs(ctx context.Context, serviceID string) (*InfoSnapshot, error)
	UpdateInfoAttrs(ctx context.Context, serviceID string, attr *InfoAttrs, update *UpdateInfo) (*InfoSnapshot, error)

	GetRuntimeAttrs(ctx context.Context, serviceID string) (*RuntimeSnapshot, error)
	UpdateRuntimeAttrs(ctx context.Context, serviceID string, attr *RuntimeAttrs, update *UpdateInfo) (*RuntimeSnapshot, error)

	GetAuthAttrs(ctx context.Context, serviceID string) (*AuthSnapshot, error)
	UpdateAuthAttrs(ctx context.Context, serviceID string, attr *AuthAttrs, update *UpdateInfo) (*AuthSnapshot, error)
}
