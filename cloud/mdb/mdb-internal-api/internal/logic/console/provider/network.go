package provider

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

func translateNetworkErrors(err error, notFoundMessage string) error {
	if semErr := semerr.AsSemanticError(err); semErr != nil {
		if semerr.IsNotFound(err) {
			return semerr.FailedPrecondition(notFoundMessage)
		}
		// we should not transparently show errors to user
		// - Authentication, Authorization - it's problems with our creds
		// - FailedPrecondition, AlreadyExists - unexpected (in our Get calls)
		return semerr.WhitelistErrors(err, semerr.SemanticUnavailable, semerr.SemanticInvalidInput)
	}
	return err
}

func (c *Console) GetNetworksByCloudExtID(ctx context.Context, cloudExtID string) ([]string, error) {
	ctx, sess, err := c.sessions.Begin(ctx, sessions.ResolveByCloud(cloudExtID, models.PermMDBAllRead), sessions.WithPrimary())
	if err != nil {
		return nil, err
	}
	defer c.sessions.Rollback(ctx)

	networks, err := c.network.GetNetworksByCloudID(ctx, sess.FolderCoords.CloudExtID)
	if err != nil {
		return nil, translateNetworkErrors(err, fmt.Sprintf("networks for cloud %q not found", sess.FolderCoords.CloudExtID))
	}

	return networks, nil
}
