package resmanager

import (
	"context"

	"a.yandex-team.ru/library/go/core/xerrors"
)

var ErrFolderNotFound = xerrors.NewSentinel("not found")

// ResolveFolder is simpler than ResolverFolders when you need to resolve a single folder.
func ResolveFolder(ctx context.Context, c Client, folderExtID string) (ResolvedFolder, error) {
	folders, err := c.ResolveFolders(ctx, []string{folderExtID})
	if err != nil {
		return ResolvedFolder{}, err
	}

	for _, folder := range folders {
		if folder.FolderExtID == folderExtID {
			return folder, nil
		}
	}

	return ResolvedFolder{}, ErrFolderNotFound.WithStackTrace()
}
