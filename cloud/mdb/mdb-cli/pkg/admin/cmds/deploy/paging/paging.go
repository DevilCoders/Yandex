package paging

import (
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
)

// TODO: move these flag types to separate package

// StringFlag ...
type StringFlag struct {
	Name    string
	Value   string
	Default string
}

// Int64Flag ...
type Int64Flag struct {
	Name    string
	Value   int64
	Default int64
}

var (
	flagSize      = Int64Flag{Name: "size", Default: 100}
	flagToken     = StringFlag{Name: "token"}
	flagSortOrder = StringFlag{Name: "sortorder"}
)

// Paging returns paging object based on flag values
func Paging() deployapi.Paging {
	return deployapi.Paging{Size: flagSize.Value, Token: flagToken.Value, SortOrder: models.ParseSortOrder(flagSortOrder.Value)}
}

// Register paging arguments
func Register(fs *pflag.FlagSet) {
	fs.Int64Var(
		&flagSize.Value,
		flagSize.Name,
		flagSize.Default,
		"paging size (number of entries to return)",
	)

	fs.StringVar(
		&flagToken.Value,
		flagToken.Name,
		flagToken.Default,
		"paging token (was returned on previous iteration if there was any)",
	)

	fs.StringVar(
		&flagSortOrder.Value,
		flagSortOrder.Name,
		flagSortOrder.Default,
		"Sorting order (valid values are 'asc, desc')",
	)
}
