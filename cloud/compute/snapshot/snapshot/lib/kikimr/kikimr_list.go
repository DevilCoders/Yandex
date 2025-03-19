package kikimr

import (
	"bytes"
	"database/sql"
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"

	"go.uber.org/zap"
	"golang.org/x/net/context"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
)

var kikimrListFields = []string{
	"id", "base", "state", "created", "size", "realsize", "metadata",
	"organization", "project", "disk", "public", "name",
	"description", "productid", "imageid", "statedescription",
}

func (st *kikimrstorage) List(ctx context.Context, r *storage.ListRequest) (l []common.SnapshotInfo, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	// TODO: indices

	// Check request validity
	sort, err := newSorting(ctx, r.Sort)
	if err != nil {
		return nil, err
	}

	where, args := buildListSelector(r, "se.")

	var q bytes.Buffer
	q.WriteString("SELECT ")
	q.WriteString("0, ")
	q.WriteString(buildListFields(kikimrListFields, "se.", map[string]string{"statedescription": "sesd."}))
	q.WriteString(" FROM ")
	if r.Project != "" {
		q.WriteString("$snapshotsext_project AS sep INNER JOIN ")
		q.WriteString("$snapshotsext AS se ON sep.id = se.id LEFT JOIN ")
	} else {
		q.WriteString("$snapshotsext AS se LEFT JOIN ")
	}
	q.WriteString("$snapshotsext_statedescription")
	q.WriteString(" AS sesd ON se.id = sesd.id ")
	q.WriteString(" WHERE ")
	q.WriteString(where)
	q.WriteString(sort.OrderByID("se."))

	err = st.retryWithTx(ctx, "queryList", func(tx Querier) error {
		l, err = st.queryList(ctx, tx, q.String(), args, r.N)
		return err
	})
	return
}

func (st *kikimrstorage) ListGC(ctx context.Context, r *storage.GCListRequest) (l []common.SnapshotInfo, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	err = st.retryWithTx(ctx, "ListGCImpl", func(tx Querier) error {
		l, err = st.ListGCImpl(ctx, tx, r)
		return err
	})
	return
}

func (st *kikimrstorage) ListBases(ctx context.Context, r *storage.BaseListRequest) (l []common.SnapshotInfo, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	err = st.retryWithTx(ctx, "ListBasesImpl", func(tx Querier) error {
		l, err = st.ListBasesImpl(ctx, tx, r)
		return err
	})
	return
}

func (st *kikimrstorage) ListChangedChildren(ctx context.Context, id string) (l []common.ChangedChild, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	err = st.retryWithTx(ctx, "ListChildrenImpl", func(tx Querier) error {
		l, err = st.ListChangedChildrenImpl(ctx, tx, id)
		return err
	})
	return
}

func (st *kikimrstorage) queryList(ctx context.Context, tx Querier, query string, args []interface{}, n int64) ([]common.SnapshotInfo, error) {
	var snapshots []common.SnapshotInfo
	err := st.selectPartial(ctx, tx, query, args, n, func(rows SQLRows) error {
		var sh common.SnapshotInfo
		var i int64

		err := rows.Scan(&i,
			&sh.ID, nullString{&sh.Base}, &sh.State.Code, kikimrTime{&sh.Created}, &sh.Size, &sh.RealSize, &sh.Metadata,
			&sh.Organization, &sh.ProjectID, &sh.Disk, &sh.Public, &sh.Name,
			&sh.Description, &sh.ProductID, &sh.ImageID, nullString{&sh.State.Description})
		if err != nil {
			log.G(ctx).Error("queryList: Scanning to SnapshotInfo failed", zap.Error(err))
			return kikimrError(err)
		}

		// TODO: sharing
		snapshots = append(snapshots, sh)
		return nil
	}, nil)
	if err != nil {
		log.G(ctx).Error("queryList: selectPartial failed", zap.Error(err))
		return nil, err
	}

	err = st.loadChanges(ctx, tx, snapshots)
	if err != nil {
		return nil, err
	}

	return snapshots, misc.ErrNoCommit
}

func (st *kikimrstorage) filterBaseList(ctx context.Context, tx Querier, id, tree, query string) ([]string, error) {
	// query must expect {tree, snapshotID} as arguments.
	lastID, prevLastID := "", "bad" // Special mark to handle no select results.
	baseMap := map[string]string{}
	args := []interface{}{sql.Named("tree", tree), sql.Named("id", lastID)}
	err := st.selectPartial(ctx, tx, query, args, -1,
		func(rows SQLRows) error {
			var snapshotID, base string
			err := rows.Scan(&snapshotID, &base)
			if err != nil {
				log.G(ctx).Error("filterBaseList: scan failed", zap.Error(err))
				return kikimrError(err)
			}
			baseMap[snapshotID] = base
			lastID = snapshotID
			return nil
		},
		func() (q string, a []interface{}) {
			if lastID == prevLastID {
				// We got no results this time.
				return "", nil
			}
			prevLastID = lastID
			return query, []interface{}{sql.Named("tree", tree), sql.Named("id", lastID)}
		})
	if err != nil {
		return nil, err
	}

	var result []string
	for id != "" {
		result = append(result, id)
		id = baseMap[id]
	}
	return result, nil
}

func (st *kikimrstorage) getDeltaBaseList(ctx context.Context, tx Querier, id, tree string) ([]string, error) {
	// TODO: Filter by depth
	query := "#PREPARE " +
		"DECLARE $tree AS Utf8; " +
		"DECLARE $id AS Utf8; " +
		"SELECT st.id, s.base FROM $snapshots_tree AS st " +
		"INNER JOIN $snapshots AS s ON st.id = s.id " +
		"WHERE st.tree = $tree AND st.id > $id " +
		"ORDER BY st.id"
	return st.filterBaseList(ctx, tx, id, tree, query)
}

func (st *kikimrstorage) getSnapshotBaseList(ctx context.Context, tx Querier, id, tree string) ([]string, error) {
	// TODO: Filter by depth
	query := "#PREPARE " +
		"DECLARE $tree AS Utf8; " +
		"DECLARE $id AS Utf8; " +
		"SELECT setr.id, se.base FROM $snapshotsext_tree AS setr " +
		"INNER JOIN $snapshotsext AS se ON setr.id = se.id " +
		"WHERE setr.tree = $tree AND setr.id > $id " +
		"ORDER BY setr.id"
	return st.filterBaseList(ctx, tx, id, tree, query)
}

func (st *kikimrstorage) loadChanges(ctx context.Context, tx Querier, l []common.SnapshotInfo) error {
	if len(l) == 0 {
		return nil
	}

	var subq bytes.Buffer
	args := make([]interface{}, 0, len(l))
	for i := range l {
		if i > 0 {
			subq.WriteString(" UNION ALL ")
		}
		subq.WriteString(fmt.Sprintf("SELECT %d AS i, $%d AS id", i, len(args)+1))
		args = append(args, l[i].ID)
	}

	query := "$subq = (" + subq.String() + "); " +
		"SELECT sq.i, sc.`timestamp`, sc.realsize " +
		"FROM $subq AS sq INNER JOIN $sizechanges AS sc ON sq.id = sc.id " +
		"ORDER BY sq.i ASC, sc.`timestamp` ASC"
	err := st.selectPartial(ctx, tx, query, args, -1, func(rows SQLRows) error {
		var timestamp time.Time
		var i, realsize int64
		if err := rows.Scan(&i, kikimrTime{&timestamp}, &realsize); err != nil {
			log.G(ctx).Error("loadChanges: scan changes failed", zap.Error(err))
			return kikimrError(err)
		}
		l[i].Changes = append(l[i].Changes, common.SizeChange{
			Timestamp: timestamp,
			RealSize:  realsize,
		})
		return nil
	}, nil)
	if err != nil {
		log.G(ctx).Error("loadChanges: selectPartial failed", zap.Error(err))
	}
	return err
}

func (st *kikimrstorage) ListBasesImpl(ctx context.Context, tx Querier, r *storage.BaseListRequest) (resInfo []common.SnapshotInfo, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	var tree, state string
	err := st.getSnapshotFields(ctx, tx, r.ID, []string{"tree", "state"}, []interface{}{&tree, &state})
	if err != nil {
		return nil, err
	}

	bases, err := st.getSnapshotBaseList(ctx, tx, r.ID, tree)
	if err != nil {
		return nil, err
	}

	if r.N != 0 && int64(len(bases)) > r.N {
		bases = bases[:r.N]
	}

	var subq bytes.Buffer
	args := make([]interface{}, 0, len(bases))
	for i, b := range bases {
		if i > 0 {
			subq.WriteString(" UNION ALL ")
		}
		subq.WriteString(fmt.Sprintf("SELECT %d AS i, $%d AS id", i, len(args)+1))
		args = append(args, b)
	}

	var q bytes.Buffer
	q.WriteString("$subq = (")
	q.WriteString(subq.String())
	q.WriteString("); ")
	q.WriteString("SELECT sq.i, ")
	q.WriteString(buildListFields(kikimrListFields, "se.", map[string]string{"statedescription": "sesd."}))
	q.WriteString(" FROM $subq AS sq")
	q.WriteString(" INNER JOIN $snapshotsext AS se ON sq.id = se.id")
	q.WriteString(" LEFT JOIN $snapshotsext_statedescription AS sesd on se.id = sesd.id")
	q.WriteString(" ORDER BY sq.i ASC")

	return st.queryList(ctx, tx, q.String(), args, 0)
}

func (st *kikimrstorage) ListGCImpl(ctx context.Context, tx Querier, r *storage.GCListRequest) (snapshotInfo []common.SnapshotInfo, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	// TODO: indices
	var args []interface{}
	var clauses []string

	args = append(args, storage.StateDeleted)

	if r.FailedCreation != 0 {
		args = append(args, storage.StateCreating, r.FailedCreation.Nanoseconds()/1000)
		clauses = append(clauses, fmt.Sprintf("(se.state = $%v and se.created < YQL::Now() - $%v)", len(args)-1, len(args)))
	}
	if r.FailedConversion != 0 {
		args = append(args, storage.StateFailed, r.FailedConversion.Nanoseconds()/1000)
		clauses = append(clauses, fmt.Sprintf("(se.state = $%v and se.created < YQL::Now() - $%v)", len(args)-1, len(args)))

		args = append(args, storage.StateRogueChunks, r.FailedConversion.Nanoseconds()/1000)
		clauses = append(clauses, fmt.Sprintf("(se.state = $%v and se.created < YQL::Now() - $%v)", len(args)-1, len(args)))
	}
	if r.FailedDeletion != 0 {
		args = append(args, storage.StateDeleting, r.FailedDeletion.Nanoseconds()/1000)
		clauses = append(clauses, fmt.Sprintf("(se.state = $%v and se.deleted < YQL::Now() - $%v)", len(args)-1, len(args)))
	}
	if r.Tombstone != 0 {
		args = append(args, storage.StateDeleted, r.Tombstone.Nanoseconds()/1000)
		clauses = append(clauses, fmt.Sprintf("(se.state = $%v and se.deleted < YQL::Now() - $%v)", len(args)-1, len(args)))
	}

	q := "$subq = (" +
		" SELECT IF(tombstone, SOME(se.id), " +
		"  IF(MIN_BY(se.id, se.deleted) IS NOT NULL, MIN_BY(se.id, se.deleted), SOME(se.id))" +
		" ) AS id, segc.chain AS chain, tombstone " +
		" FROM $snapshotsext_gc AS segc " +
		" INNER JOIN $snapshotsext AS se ON segc.id = se.id " +
		" GROUP BY segc.chain, se.state = $1 AS tombstone" +
		"); " +
		"SELECT 0, " + buildListFields(kikimrListFields, "se.", nil) + " " +
		"FROM $subq AS subq INNER JOIN $snapshotsext AS se " +
		"ON subq.id = se.id " +
		"WHERE " + strings.Join(clauses, " OR ") + " ORDER BY se.id ASC"

	return st.queryList(ctx, tx, q, args, int64(r.N))
}

func (st *kikimrstorage) ListChangedChildrenImpl(ctx context.Context, tx Querier, id string) (l []common.ChangedChild, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	var state string
	err = st.getSnapshotFields(ctx, tx, id, []string{"state"}, []interface{}{&state})
	if err != nil {
		return nil, err
	}
	if state != storage.StateDeleted {
		log.G(ctx).Debug("ListChangedChildrenImpl: snapshot is not deleted")
		return nil, misc.ErrInvalidState
	}

	q := "#PREPARE " +
		"DECLARE $base AS Utf8; " +
		"SELECT id, `timestamp`, realsize FROM $changed_children WHERE base = $base"

	err = st.selectPartial(ctx, tx, q, []interface{}{sql.Named("base", id)}, -1, func(rows SQLRows) error {
		var child common.ChangedChild

		err := rows.Scan(&child.ID, kikimrTime{&child.Timestamp}, &child.RealSize)
		if err != nil {
			log.G(ctx).Error("ListChangedChildrenImpl: scanning to ChangedChild failed", zap.Error(err))
			return kikimrError(err)
		}

		l = append(l, child)
		return nil
	}, nil)
	if err != nil {
		log.G(ctx).Error("ListChangedChildrenImpl: selectPartial failed", zap.Error(err))
		return nil, err
	}
	return l, nil
}
