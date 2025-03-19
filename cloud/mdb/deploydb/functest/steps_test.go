package functest

import (
	"strings"

	"github.com/jackc/pgtype"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
)

var (
	queryCreateGroup = sqlutil.Stmt{
		Name: "CreateGroup",
		Query: "SELECT * FROM code.create_group(" +
			"i_name => :group)",
	}
	queryCreateMaster = sqlutil.Stmt{
		Name: "CreateMaster",
		Query: "SELECT * FROM code.create_master(" +
			"i_fqdn => :fqdn, " +
			"i_deploy_group => :group," +
			"i_is_open => :is_open," +
			"i_description => 'foo')",
	}
	queryUpdateMasterCheck = sqlutil.Stmt{
		Name:  "UpdateMasterCheck",
		Query: "SELECT * FROM code.update_master_check(:fqdn, 'autochecker', :is_alive)",
	}
	queryCreateMinion = sqlutil.Stmt{
		Name: "CreateMinion",
		Query: "SELECT * FROM code.create_minion(" +
			"i_fqdn => :fqdn," +
			"i_deploy_group => :group," +
			"i_auto_reassign => true)",
	}
	queryCreateShipment = sqlutil.Stmt{
		Name: "CreateShipment",
		Query: "SELECT * FROM code.create_shipment(" +
			"i_commands => ARRAY[CAST((:command, NULL, NULL) AS code.command_def)]," +
			"i_fqdns => :fqdns," +
			"i_parallel => 1," +
			"i_stop_on_error_count => 0," +
			"i_timeout => INTERVAL '1 hour')",
	}
)

func (tctx *testContext) queryNode(query sqlutil.Stmt, args map[string]interface{}) error {
	_, err := sqlutil.QueryNode(tctx.S.TC.Context(), tctx.S.Node, query, args, sqlutil.NopParser, tctx.S.L)
	return err
}

func (tctx *testContext) group(arg1 string) error {
	return tctx.queryNode(queryCreateGroup, map[string]interface{}{"group": arg1})
}

func (tctx *testContext) openAliveMasterInGroup(arg1, arg2 string) error {
	return tctx.createMaster(arg1, arg2, true, true)
}

func (tctx *testContext) closedAliveMasterInGroup(arg1, arg2 string) error {
	return tctx.createMaster(arg1, arg2, false, true)
}

func (tctx *testContext) createMaster(fqdn, group string, isOpen, isAlive bool) error {

	if err := tctx.queryNode(queryCreateMaster, map[string]interface{}{"fqdn": fqdn, "group": group, "is_open": isOpen}); err != nil {
		return err
	}
	return tctx.queryNode(queryUpdateMasterCheck, map[string]interface{}{"fqdn": fqdn, "is_alive": isAlive})
}

func (tctx *testContext) minionInGroup(arg1, arg2 string) error {
	return tctx.queryNode(queryCreateMinion, map[string]interface{}{"fqdn": arg1, "group": arg2})
}

func (tctx *testContext) shipmentOn(arg1, arg2 string) error {
	spl := strings.Split(arg2, ",")
	for i := range spl {
		spl[i] = strings.TrimSpace(spl[i])
	}

	fqdns := &pgtype.TextArray{}
	if err := fqdns.Set(spl); err != nil {
		return err
	}

	return tctx.queryNode(queryCreateShipment, map[string]interface{}{"command": arg1, "fqdns": fqdns})
}
