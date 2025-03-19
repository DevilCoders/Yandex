package pg

import (
	"context"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
)

var (
	queryCreateGroup = sqlutil.Stmt{
		Name:  "CreateGroup",
		Query: "SELECT * FROM code.create_group(i_name => :name)",
	}

	querySelectGroup = sqlutil.Stmt{
		Name:  "SelectGroup",
		Query: "SELECT * FROM code.get_group(i_name => :name)",
	}

	querySelectGroups = sqlutil.Stmt{
		Name: "SelectGroups",
		Query: "SELECT * FROM code.get_groups(" +
			"i_limit => :limit, " +
			"i_last_group_id => :last_group_id, " +
			"i_ascending => :ascending)",
	}
)

func (b *backend) CreateGroup(ctx context.Context, name string) (models.Group, error) {
	var group groupModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&group)
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.PrimaryChooser(),
		queryCreateGroup,
		map[string]interface{}{"name": name},
		parser,
		b.logger,
	); err != nil {
		return models.Group{}, errorToSemErr(err)
	}

	return groupFromDB(group), nil
}

func (b *backend) Group(ctx context.Context, name string) (models.Group, error) {
	var group groupModel
	parser := func(rows *sqlx.Rows) error {
		return rows.StructScan(&group)
	}

	count, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		querySelectGroup,
		map[string]interface{}{"name": name},
		parser,
		b.logger,
	)
	if err != nil {
		return models.Group{}, errorToSemErr(err)
	}
	if count == 0 {
		return models.Group{}, semerr.NotFoundf("group %q not found", name)
	}

	return groupFromDB(group), nil
}

func (b *backend) Groups(ctx context.Context, sortOrder models.SortOrder, limit int64, lastGroupID optional.Int64) ([]models.Group, error) {
	var groups []models.Group
	parser := func(rows *sqlx.Rows) error {
		var group groupModel
		if err := rows.StructScan(&group); err != nil {
			return err
		}

		groups = append(groups, groupFromDB(group))
		return nil
	}

	args := map[string]interface{}{
		"limit":     limit,
		"ascending": sortOrderToDB(sortOrder),
	}
	if lastGroupID.Valid {
		args["last_group_id"] = lastGroupID.Must()
	} else {
		args["last_group_id"] = nil
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		querySelectGroups,
		args,
		parser,
		b.logger,
	); err != nil {
		return nil, errorToSemErr(err)
	}

	return groups, nil
}
