package pg

import (
	"time"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/mdb-search-producer/internal/metadb"
)

type unsentRow struct {
	ID        int64     `db:"sq_id"`
	QueueID   int64     `db:"queue_id"`
	Doc       string    `db:"doc"`
	CreatedAt time.Time `db:"created_at"`
}

func (r unsentRow) format() metadb.UnsentSearchDoc {
	return metadb.UnsentSearchDoc{
		QueueID:   r.QueueID,
		Doc:       r.Doc,
		CreatedAt: r.CreatedAt,
	}
}

type unsetRowsParser struct {
	ret []metadb.UnsentSearchDoc
}

func (p *unsetRowsParser) parse(rows *sqlx.Rows) error {
	var row unsentRow
	if err := rows.StructScan(&row); err != nil {
		return err
	}
	p.ret = append(p.ret, row.format())
	return nil
}

type nonEnumeratedRow struct {
	ID        int64     `db:"sq_id"`
	CreatedAt time.Time `db:"created_at"`
}

func (r nonEnumeratedRow) format() metadb.NonEnumeratedSearchDoc {
	return metadb.NonEnumeratedSearchDoc{
		ID:        r.ID,
		CreatedAt: r.CreatedAt,
	}
}
