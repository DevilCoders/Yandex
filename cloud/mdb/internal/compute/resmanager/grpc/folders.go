package grpc

import (
	"context"

	resmanv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/resourcemanager/v1"
	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *Client) ResolveFolders(ctx context.Context, folderExtIDs []string) ([]resmanager.ResolvedFolder, error) {
	resp, err := c.folders.Resolve(ctx, &resmanv1.ResolveFoldersRequest{FolderIds: folderExtIDs})
	if err != nil {
		return nil, xerrors.Errorf("resolve folders %q: %w", folderExtIDs, grpcerr.SemanticErrorFromGRPC(err))
	}

	res := make([]resmanager.ResolvedFolder, 0, len(resp.ResolvedFolders))
	for _, folder := range resp.ResolvedFolders {
		f := resmanager.ResolvedFolder{
			CloudExtID:        folder.CloudId,
			FolderExtID:       folder.Id,
			OrganizationExtID: folder.OrganizationId,
			// Ignore folder.ResourcePath for now
		}

		res = append(res, f)
	}
	return res, nil
}

func (c *Client) CheckServiceAccountRole(ctx context.Context, iamToken, folderExtID, serviceAccountID, roleID string) (bool, error) {
	return false, semerr.NotImplemented("not implemented")
}
