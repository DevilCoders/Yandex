package functest

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/reindexer/logic"
)

func (tctx *testContext) weRunMDBSearchReindexer() error {
	reindex, err := logic.New(tctx.InternalAPI.Config, tctx.L, 1)
	if err != nil {
		return fmt.Errorf("init reindexer: %w", err)
	}
	return reindex.Run(tctx.TC.Context())
}
