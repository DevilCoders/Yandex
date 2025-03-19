package clickhouse

import (
	"database/sql"
	"fmt"

	"github.com/yandex/pandora/core/aggregator/netsample"
)

func (g *Gun) Select1(ammo *Ammo) {
	var result *sql.Rows

	code := 200
	var err error
	transSample := netsample.Acquire("select1")

	sample := netsample.Acquire("select")
	result, err = g.connect.Query("SELECT 1")
	if err != nil {
		code = 500
		fmt.Println(err)
	}
	result.Close()
	sample.SetProtoCode(200)
	g.aggr.Report(sample)

	defer func() {
		transSample.SetProtoCode(code)
		g.aggr.Report(transSample)
	}()
}
