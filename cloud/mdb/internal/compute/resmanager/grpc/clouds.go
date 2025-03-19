package grpc

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/access"
	resmanv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/resourcemanager/v1"
	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *Client) PermissionStages(ctx context.Context, cloudExtID string) ([]string, error) {
	res, err := c.clouds.GetPermissionStages(ctx, &resmanv1.GetPermissionStagesRequest{CloudId: cloudExtID})
	if err != nil {
		return nil, xerrors.Errorf("permission stages %q: %w", cloudExtID, grpcerr.SemanticErrorFromGRPC(err))
	}

	return res.PermissionStages, nil
}

func (c *Client) ListAccessBindings(ctx context.Context, resourceID string, private bool) ([]resmanager.AccessBinding, error) {
	var bindings []resmanager.AccessBinding
	var pageToken string
	for {
		resp, err := c.clouds.ListAccessBindings(ctx, &access.ListAccessBindingsRequest{
			ResourceId:  resourceID,
			PageToken:   pageToken,
			PrivateCall: private,
		})
		if err != nil {
			return nil, xerrors.Errorf("list access bindings %q: %w", resourceID, grpcerr.SemanticErrorFromGRPC(err))
		}
		for _, s := range resp.AccessBindings {
			b := resmanager.AccessBinding{
				RoleID: s.RoleId,
				Subject: resmanager.Subject{
					ID:   s.Subject.Id,
					Type: s.Subject.Type,
				},
			}
			bindings = append(bindings, b)
		}

		if resp.NextPageToken == "" {
			break
		}
		pageToken = resp.NextPageToken
	}

	return bindings, nil
}

func (c *Client) Cloud(ctx context.Context, cloudExtID string) (resmanager.Cloud, error) {
	cloud, err := c.clouds.Get(ctx, &resmanv1.GetCloudRequest{CloudId: cloudExtID})
	if err != nil {
		return resmanager.Cloud{}, xerrors.Errorf("get cloud %q: %w", cloudExtID, grpcerr.SemanticErrorFromGRPC(err))
	}
	return resmanager.Cloud{
		CloudID: cloud.Id,
		Name:    cloud.Name,
		Status:  resmanager.CloudStatus(cloud.Status),
	}, nil
}
