package functest

import (
	"database/sql"
	"encoding/json"
	"fmt"
	"path"
	"reflect"
	"strconv"
	"strings"
	"time"

	"github.com/DATA-DOG/godog/gherkin"
	"github.com/PaesslerAG/jsonpath"
	"github.com/jackc/pgtype"
	"github.com/jmoiron/sqlx"
	"github.com/opentracing/opentracing-go"
	"github.com/santhosh-tekuri/jsonschema/v5"
	"github.com/spf13/cast"

	"a.yandex-team.ru/cloud/mdb/internal/diff"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/tracing"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
	"a.yandex-team.ru/library/go/test/yatest"
)

var (
	queryAcquireTask = sqlutil.Stmt{
		Name:  "AcquireTask",
		Query: "SELECT code.acquire_task('dummy', :task_id)",
	}
	queryFinishTask = sqlutil.Stmt{
		Name:  "FinishTask",
		Query: "SELECT code.finish_task('dummy', :task_id, :res, CAST('{}' AS jsonb), '')",
	}
	queryTaskCreatedAt = sqlutil.Stmt{
		Name:  "TaskCreatedAt",
		Query: "UPDATE dbaas.worker_queue SET create_ts = :created_at WHERE task_id = :task_id",
	}
	querySelectTaskArgs = sqlutil.Stmt{
		Name:  "SelectTaskArgs",
		Query: "SELECT task_args FROM dbaas.worker_queue WHERE task_id = :task_id",
	}
	querySelectTaskType = sqlutil.Stmt{
		Name:  "SelectTaskType",
		Query: "SELECT task_type FROM dbaas.worker_queue WHERE task_id = :task_id",
	}
	querySelectFullTask = sqlutil.Stmt{
		Name:  "SelectFullTask",
		Query: "SELECT * FROM dbaas.worker_queue WHERE task_id = :task_id",
	}
	querySelectWorkerQueueEvents = sqlutil.Stmt{
		Name:  "SelectWorkerQueueEvents",
		Query: "SELECT data FROM dbaas.worker_queue_events WHERE task_id = :task_id",
	}
	querySelectWorkerQueueTracing = sqlutil.Stmt{
		Name:  "SelectWorkerQueueTracing",
		Query: "SELECT tracing FROM dbaas.worker_queue WHERE task_id = :task_id",
	}
	queryChangeFQDNAndGeo = sqlutil.Stmt{
		Name: "ChangeFQDNAndGeo",
		Query: `WITH old_pillar AS (
            DELETE FROM dbaas.pillar
             WHERE fqdn = :old_fqdn
            RETURNING *
        ),
            changed_host AS (
            UPDATE dbaas.hosts
               SET geo_id = (SELECT geo.geo_id
                            FROM dbaas.geo
                            WHERE name = :new_geo),
                   fqdn = :new_fqdn
             WHERE fqdn = :old_fqdn
            RETURNING * )
        INSERT INTO dbaas.pillar (fqdn, value)
        SELECT ch.fqdn, op.value
          FROM old_pillar op, changed_host ch`,
	}
	queryAllRevsCommttedBefore = sqlutil.Stmt{
		Name: "AllRevsCommttedBefore",
		Query: `UPDATE dbaas.clusters_changes
           SET committed_at = CAST(:ts AS timestamptz) - make_interval(CAST(rn AS int))
          FROM (
            SELECT rev o_rev,
                   cid o_cid,
                   row_number() OVER (ORDER BY rev DESC) AS rn
              FROM dbaas.clusters_changes
             WHERE cid = :cid) nr
         WHERE rev = o_rev
           AND cid = o_cid`,
	}
	querySelectLastDoc = sqlutil.Stmt{
		Name:  "SelectLastDoc",
		Query: "SELECT doc FROM dbaas.search_queue ORDER BY created_at DESC, sq_id DESC LIMIT 1",
	}
	queryAddFFlagValidResources = sqlutil.Stmt{
		Name: "AddFFlagValidResources",
		// language=PostgreSQL
		Query: `UPDATE dbaas.valid_resources
SET feature_flag = :feature_flag
WHERE geo_id = (SELECT geo.geo_id FROM dbaas.geo WHERE name = :geo_ext_id)`}
	querySaveValidResources = sqlutil.Stmt{
		Name: "SaveValidResources",
		// language=PostgreSQL
		Query: `CREATE TABLE tmp_valid_resources AS SELECT * from dbaas.valid_resources`}
	queryDropSavedValidResources = sqlutil.Stmt{
		Name: "DropSavedValidResources",
		// language=PostgreSQL
		Query: `DROP TABLE IF EXISTS tmp_valid_resources `}
	queryDeleteValidResources = sqlutil.Stmt{
		Name: "DeleteValidResources",
		// language=PostgreSQL
		Query: `DELETE FROM dbaas.valid_resources`}
	queryRestoreValidResources = sqlutil.Stmt{
		Name: "SaveValidResources",
		// language=PostgreSQL
		Query: `INSERT INTO dbaas.valid_resources SELECT * FROM tmp_valid_resources`}
	queryShrinkValidResourcesByGeo = sqlutil.Stmt{
		Name: "ShrinkValidResourcesByGeo",
		// language=PostgreSQL
		Query: `DELETE FROM dbaas.valid_resources WHERE geo_id NOT IN (SELECT geo_id FROM dbaas.geo WHERE name = ANY(:shrink_value))`}
	queryShrinkValidResourcesByDiskType = sqlutil.Stmt{
		Name: "ShrinkValidResourcesByDiskType",
		// language=PostgreSQL
		Query: `DELETE FROM dbaas.valid_resources WHERE disk_type_id NOT IN (SELECT disk_type.disk_type_id FROM dbaas.disk_type WHERE disk_type_ext_id = ANY(:shrink_value))`}
	queryShrinkValidResourcesByFlavorName = sqlutil.Stmt{
		Name: "ShrinkValidResourcesByFlavorName",
		// language=PostgreSQL
		Query: `DELETE FROM dbaas.valid_resources WHERE flavor NOT IN (SELECT flavors.id FROM dbaas.flavors WHERE flavors.name = ANY(:shrink_value))`}
	querySelectDocByNum = sqlutil.Stmt{
		Name:  "SelectDocByNum",
		Query: "SELECT doc FROM dbaas.search_queue ORDER BY created_at ASC, sq_id ASC LIMIT 1 OFFSET :num",
	}
	querySearchQueueSize = sqlutil.Stmt{
		Name:  "SearchQueueSize",
		Query: "SELECT count(*) FROM dbaas.search_queue",
	}

	querySelectCountClusterHosts = sqlutil.Stmt{
		Name: "SelectClusterHosts",
		Query: `select count(*) from dbaas.clusters
c join dbaas.subclusters sc on c.cid = sc.cid join dbaas.hosts h on h.subcid = sc.subcid
where c.cid =:cid`,
	}

	insertDefaultFeatureFlag = sqlutil.Stmt{
		Name:  "InsertDefaultFeatureFlag",
		Query: "INSERT INTO dbaas.default_feature_flags (flag_name) VALUES (:feature_flag)",
	}

	queryInsertCloudFeatureFlag = sqlutil.Stmt{
		Name: "InsertCloudFeatureFlag",
		Query: `INSERT INTO dbaas.cloud_feature_flags (cloud_id, flag_name)
				(SELECT c.cloud_id, :feature_flag FROM dbaas.clouds as c WHERE c.cloud_ext_id = :cloud_ext_id )`,
	}

	queryAddCloud = sqlutil.Stmt{
		Name: "AddCloud",
		Query: `
        SELECT *
          FROM code.add_cloud(
		    i_cloud_ext_id   => :cloud_ext_id,
		    i_quota          => code.make_quota(
			  i_cpu		   => :cpu,
			  i_memory     => :memory,
			  i_ssd_space  => :ssd,
			  i_hdd_space  => :hdd,
			  i_clusters   => :clusters
		    ),
            i_x_request_id => ''
        )`,
	}

	queryAddFolder = sqlutil.Stmt{
		Name: "AddFolder",
		Query: `
        INSERT INTO dbaas.folders (folder_ext_id, cloud_id)
		(SELECT :folder_ext_id, c.cloud_id FROM dbaas.clouds as c WHERE c.cloud_ext_id = :cloud_ext_id)`,
	}

	queryAddManagedBackup = sqlutil.Stmt{
		Name: "AddManagedBackup",
		Query: `
		INSERT INTO dbaas.backups (backup_id, cid, subcid, shard_id, status, method, initiator, created_at, delayed_until, started_at, finished_at, updated_at, scheduled_date, metadata)
		VALUES
		(
			:backup_id,
			:cid,
			:subcid,
			:shard_id,
			CAST(:status AS dbaas.backup_status),
			CAST(:method as dbaas.backup_method),
			CAST(:initiator as dbaas.backup_initiator),
			CAST(:ts as timestamptz),
			CAST(:ts as timestamptz),
			CAST(:start_ts as timestamptz),
			CAST(:stop_ts as timestamptz),
			CAST(:stop_ts as timestamptz),
			:scheduled_date,
			CAST(:metadata as jsonb)
		)`,
	}

	querySetManagedBackupStatus = sqlutil.Stmt{
		Name: "SetManagedBackupStatus",
		Query: `
        UPDATE dbaas.backups
		SET status = CAST(:status AS dbaas.backup_status)
		WHERE
		backup_id = :backup_id
		`,
	}

	queryGetLastManagedBackupStatus = sqlutil.Stmt{
		Name:  "GetLastManagedBackupStatus",
		Query: "SELECT status FROM dbaas.backups WHERE cid = :cid ORDER BY created_at DESC LIMIT 1",
	}

	queryGetManagedBackupStatusByID = sqlutil.Stmt{
		Name:  "GetLastManagedBackupStatus",
		Query: "SELECT status FROM dbaas.backups WHERE backup_id = :backup_id LIMIT 1",
	}

	queryEnableBackupService = sqlutil.Stmt{
		Name: "EnableBackupService",
		Query: `
        SELECT * FROM code.set_backup_service_use(
		    i_cid => :cid,
			i_val => true
		)`,
	}
	queryCreateInitialBackup = sqlutil.Stmt{
		Name:  "CompleteAllClusterBackups",
		Query: `UPDATE dbaas.backups SET status = 'DONE' WHERE cid = :cid`,
	}

	querySelectBackupsCount = sqlutil.Stmt{
		Name:  "SelectBackupsCount",
		Query: "SELECT count(*) FROM dbaas.backups",
	}

	querySetSecurityGroups = sqlutil.Stmt{
		Name: "SetSecurityGroups",
		Query: `
		INSERT INTO dbaas.sgroups
			(sg_ext_id, cid, sg_type)
		SELECT sg_ext_id, :cid, 'user'
          FROM unnest(CAST(:sg_ids AS text[])) AS sg_ext_id`,
	}

	queryClearSecurityGroups = sqlutil.Stmt{
		Name: "ClearSecurityGroups",
		Query: `
		DELETE FROM dbaas.sgroups WHERE cid=:cid and sg_type='user'
		`,
	}

	querySelectPillarValue = sqlutil.Stmt{
		Name: "SelectPillarValue",
		Query: `
		SELECT code.easy_get_pillar(:cid,:path)
		`,
	}

	queryUpdatePillar = sqlutil.Stmt{
		Name: "UpdatePillar",
		Query: `
		SELECT code.easy_update_pillar(:cid, :path, :value)
		`,
	}
)

type metaDBContext struct {
	Event                string
	ValidResourcesShrunk bool
}

func (mctx *metaDBContext) Reset() {
	mctx.Event = ""
}

func (tctx *testContext) weRunQuery(query *gherkin.DocString) error {
	stmt := sqlutil.Stmt{
		Name:  "ANONYMOUS",
		Query: strings.Replace(query.Content, ":", "::", -1), // Escape ':' symbol or sqlx will break
	}

	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, stmt, nil, sqlutil.NopParser, &nop.Logger{})
	return err
}

func (tctx *testContext) pillarPathMatches(cid string, value string, pillarPath string) error {
	params := map[string]interface{}{
		"cid":  cid,
		"path": pillarPath,
	}
	var actual string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&actual)
	}
	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, querySelectPillarValue, params, parser, &nop.Logger{})
	if err != nil {
		return err
	}
	if actual != value {
		return xerrors.Errorf("expected %s status but has %s", value, actual)
	}
	return nil
}

func (tctx *testContext) acquiredAndFinishedByWorker(taskID string) error {
	return tctx.acquireAndCompleteTask(taskID, true)
}

func (tctx *testContext) acquiredAndFailedByWorker(taskID string) error {
	return tctx.acquireAndCompleteTask(taskID, false)
}

func (tctx *testContext) acquireAndCompleteTask(taskID string, res bool) error {
	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryAcquireTask, map[string]interface{}{"task_id": taskID}, sqlutil.NopParser, &nop.Logger{})
	if err != nil {
		return err
	}

	_, err = sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryFinishTask, map[string]interface{}{"task_id": taskID, "res": res}, sqlutil.NopParser, &nop.Logger{})
	return err
}

func (tctx *testContext) workerTaskCreatedAt(taskID, at string) error {
	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryTaskCreatedAt, map[string]interface{}{"task_id": taskID, "created_at": at}, sqlutil.NopParser, &nop.Logger{})
	return err
}

func (tctx *testContext) inWorkerQueueExistsIDWithArgsSetTo(taskID, argName, argValue string) error {
	taskArgs, err := tctx.taskArgs(taskID)
	if err != nil {
		return err
	}

	actualValue, ok := taskArgs[argName]
	if !ok {
		return xerrors.Errorf("task argument %q not found in loaded arguments: %+v", argName, taskArgs)
	}

	strActualValue, err := cast.ToStringE(actualValue)
	if err != nil {
		if reflect.TypeOf(actualValue).Kind() != reflect.Slice {
			return xerrors.Errorf("failed to convert actual value %+v to string: %w", actualValue, err)
		}
		actualValueList, err := cast.ToStringSliceE(actualValue)
		if err != nil {
			return xerrors.Errorf("failed to convert actual value %+v to slice of string: %w", actualValue, err)
		}
		strActualValue = "[" + strings.Join(actualValueList, ", ") + "]"
	}

	// We do case-insensitive comparison because right now there are incompatible type values in features like 'True' which does not map to go boolean as is
	if !strings.EqualFold(argValue, strActualValue) {
		return xerrors.Errorf("expected task argument %q to be %q but actual value is %s", argName, argValue, strActualValue)
	}

	return nil
}

func (tctx *testContext) inWorkerQueueExistsIDWithArgsContainingKeyValue(taskID, argName string, argValue *gherkin.DataTable) error {
	taskArgs, err := tctx.taskArgs(taskID)
	if err != nil {
		return err
	}

	actualMapI, ok := taskArgs[argName]
	if !ok {
		return xerrors.Errorf("task argument %q not found in loaded arguments: %+v", argName, taskArgs)
	}
	actualMap, ok := actualMapI.(map[string]interface{})
	if !ok {
		return xerrors.Errorf("task argument %q is not a map[string]: %+v", argName, actualMapI)
	}

	for _, row := range argValue.Rows {
		key := row.Cells[0].Value
		value := row.Cells[1].Value
		actualValue, ok := actualMap[key]
		if !ok {
			return xerrors.Errorf("key %q not found in loaded arguments: %+v", key, actualMap)
		}
		if cast.ToString(actualValue) != value {
			return xerrors.Errorf("expected %q for key %q, but got %q", value, key, cast.ToString(actualValue))
		}
	}
	return nil
}

func (tctx *testContext) inWorkerQueueExistsIDWithArgsContaining(taskID, argName string, argValue *gherkin.DataTable) error {
	taskArgs, err := tctx.taskArgs(taskID)
	if err != nil {
		return err
	}

	val, err := cast.ToStringSliceE(taskArgs[argName])
	if err != nil {
		return xerrors.Errorf("failed to convert actual value %+v to []string: %w", val, err)
	}

	var vals []string
	for _, row := range argValue.Rows {
		vals = append(vals, row.Cells[0].Value)
	}
	if !slices.ContainsAllStrings(val, vals) {
		return xerrors.Errorf("expected task argument %s to be %+v but actual value is %v", argName, argValue, val)
	}
	return nil
}

func (tctx *testContext) inWorkerQueueExistsIDWithArgsEmptyList(taskID, argName string) error {
	taskArgs, err := tctx.taskArgs(taskID)
	if err != nil {
		return err
	}

	val, ok := taskArgs[argName]
	if !ok {
		return xerrors.Errorf("task_args doesn't contain value %s", argName)
	}

	vals, err := cast.ToStringSliceE(val)
	if err != nil {
		return xerrors.Errorf("failed to convert actual value %+v to []string: %w", val, err)
	}

	if len(vals) > 0 {
		return xerrors.Errorf("expected task argument %s to be empty list but actual value is %v", argName, vals)
	}
	return nil
}

func (tctx *testContext) inWorkerQueueExistsIDWithoutArgs(taskID, argName string) error {
	taskArgs, err := tctx.taskArgs(taskID)
	if err != nil {
		return err
	}

	_, ok := taskArgs[argName]
	if !ok {
		return nil
	}

	return xerrors.Errorf("task_args contains value %s", argName)
}

func (tctx *testContext) taskArgs(taskID string) (map[string]interface{}, error) {
	var loaded string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&loaded)
	}

	count, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, querySelectTaskArgs, map[string]interface{}{"task_id": taskID}, parser, tctx.L)
	if err != nil {
		return nil, err
	}

	if count == 0 {
		return nil, xerrors.New("expected task but got none")
	}

	// Unmarshal actual to map
	var actual map[string]interface{}
	if err = json.Unmarshal([]byte(loaded), &actual); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal actual value %s: %w", loaded, err)
	}
	return actual, nil
}

func (tctx *testContext) forExistsEventTypeWith(taskID, eventType string, body *gherkin.DocString) error {
	data, ed, err := tctx.loadEventForTask(taskID)
	if err != nil {
		return err
	}

	if ed.MetaData.Type != eventType {
		return xerrors.Errorf("expected %q event type but has %q", eventType, ed.MetaData.Type)
	}

	// Do not check empty body
	if body.Content == "" {
		return nil
	}

	// Unmarshal expected to map
	var expected map[string]interface{}
	if err = json.Unmarshal([]byte(body.Content), &expected); err != nil {
		return xerrors.Errorf("failed to unmarshal expected value %s: %w", body.Content, err)
	}

	// Unmarshal actual to map
	var actual map[string]interface{}
	if err = json.Unmarshal([]byte(data), &actual); err != nil {
		return xerrors.Errorf("failed to unmarshal actual value %s: %w", data, err)
	}

	return diff.OptionalKeys(expected, actual)
}

func (tctx *testContext) forExistsEvent(taskID string) error {
	data, _, err := tctx.loadEventForTask(taskID)
	if err != nil {
		return err
	}

	tctx.MetaDBCtx.Event = data
	return nil
}

func (tctx *testContext) forExistsEventType(taskID, eventType string) error {
	_, ed, err := tctx.loadEventForTask(taskID)
	if err != nil {
		return err
	}

	if ed.MetaData.Type != eventType {
		return xerrors.Errorf("expected %q event type but has %q", eventType, ed.MetaData.Type)
	}

	return nil
}

func (tctx *testContext) inWorkerQueueExistsWithID(taskType, taskID string) error {
	var actual string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&actual)
	}
	count, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, querySelectTaskType, map[string]interface{}{"task_id": taskID}, parser, tctx.L)
	if err != nil {
		return err
	}

	if count == 0 {
		return xerrors.New("expected task but got none")
	}

	if actual != taskType {
		return xerrors.Errorf("expected task type %q but got %q", taskType, actual)
	}

	return nil
}

func (tctx *testContext) inWorkerQueueExistsWithIDWithDataContains(taskID string, body *gherkin.DocString) error {
	actual := map[string]interface{}{}
	parser := func(rows *sqlx.Rows) error {
		return rows.MapScan(actual)
	}
	count, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, querySelectFullTask, map[string]interface{}{"task_id": taskID}, parser, tctx.L)
	if err != nil {
		return err
	}

	if count == 0 {
		return xerrors.New("expected task but got none")
	}

	var expected map[string]interface{}
	if err := json.Unmarshal([]byte(body.Content), &expected); err != nil {
		return xerrors.Errorf("step input json unmarshal error for body %s: %w", body.Content, err)
	}

	for key, value := range expected {
		actualValue, ok := actual[key]
		if !ok {
			return xerrors.Errorf("expected task contains column %q", key)
		}

		if res := diff.Full(value, actualValue); res != nil {
			return res
		}
	}

	return nil
}

func (tctx *testContext) eventBodyDoesNotContainsPaths(taskID string, path2value *gherkin.DataTable) error {
	data, _, err := tctx.loadEventForTask(taskID)
	if err != nil {
		return err
	}

	// Unmarshal actual to map
	var actual map[string]interface{}
	if err = json.Unmarshal([]byte(data), &actual); err != nil {
		return xerrors.Errorf("failed to unmarshal actual value %s: %w", data, err)
	}

	for _, row := range path2value.Rows {
		if len(row.Cells) != 1 {
			return xerrors.Errorf("expect table with 1 columns got %d", len(row.Cells))
		}
		jsPath := row.Cells[0].Value
		value, err := jsonpath.Get(jsPath, actual)
		if err != nil {
			// ok
		} else {
			return xerrors.Errorf("event body has value on path %s: %s. Body: %s", jsPath, value, data)
		}
	}
	return nil
}

func (tctx *testContext) taskHasValidTracing(taskID string) error {
	var actual sql.NullString

	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&actual)
	}
	count, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, querySelectWorkerQueueTracing, map[string]interface{}{"task_id": taskID}, parser, tctx.L)
	if err != nil {
		return err
	}

	if count == 0 {
		return xerrors.New("expected task but got none")
	}

	if !actual.Valid {
		return xerrors.New("expect valid tracing but it is null")
	}

	spanData, err := tracing.UnmarshalTextMapCarrier([]byte(actual.String))
	if err != nil {
		return xerrors.Errorf("not a valid tracing, cause it's unmarshal failed: %w", err)
	}

	_, err = opentracing.GlobalTracer().Extract(opentracing.TextMap, spanData)
	if err != nil {
		return xerrors.Errorf("not a valid tracing, cause it's %+v extraction failed: %w", spanData, err)
	}

	return nil
}

func (tctx *testContext) forThereAreNoEvents(taskID string) error {
	count, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, querySelectWorkerQueueEvents, map[string]interface{}{"task_id": taskID}, sqlutil.NopParser, tctx.L)
	if err != nil {
		return err
	}

	if count != 0 {
		return xerrors.New("expected no events but found one")
	}

	return nil
}

func (tctx *testContext) thatEventMatchesSchema(filename string) error {
	filepath := path.Join(yatest.SourcePath("cloud/mdb/mdb-internal-api/functest/"), filename)
	schema, err := jsonschema.Compile(filepath)
	if err != nil {
		return xerrors.Errorf("failed to compile JSON schema: %w", err)
	}

	var v interface{}
	err = json.Unmarshal([]byte(tctx.MetaDBCtx.Event), &v)
	if err != nil {
		return xerrors.Errorf("failed to unmarshal JSON event: %w", err)
	}

	err = schema.Validate(v)
	if err != nil {
		return xerrors.Errorf("failed to validate JSON against schema: %#v", err)
	}

	return nil
}

func (tctx *testContext) thatSearchDocMatchesSchema(filename string) error {
	var doc string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&doc)
	}

	count, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, querySelectLastDoc, map[string]interface{}{"num": 1}, parser, &nop.Logger{})
	if err != nil {
		return err
	}
	if count == 0 {
		return xerrors.New("expected document but got none")
	}

	// Unmarshal actual to map
	var actual map[string]interface{}
	if err = json.Unmarshal([]byte(doc), &actual); err != nil {
		return xerrors.Errorf("failed to unmarshal actual value %s: %w", doc, err)
	}

	filepath := path.Join(yatest.SourcePath("cloud/search/schemas/"), filename)
	schema, err := jsonschema.Compile(filepath)
	if err != nil {
		return xerrors.Errorf("failed to compile JSON schema: %w", err)
	}

	var v interface{}
	err = json.Unmarshal([]byte(doc), &v)
	if err != nil {
		return xerrors.Errorf("failed to unmarshal JSON document: %w", err)
	}

	err = schema.Validate(v)
	if err != nil {
		return xerrors.Errorf("failed to validate JSON against schema: %#v", err)
	}

	return nil
}

var validResourcesShrink = map[string]sqlutil.Stmt{
	"flavor": queryShrinkValidResourcesByFlavorName,
	"geo":    queryShrinkValidResourcesByGeo,
	"disk":   queryShrinkValidResourcesByDiskType,
}

func (tctx *testContext) saveValidResources() error {
	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, querySaveValidResources, map[string]interface{}{}, func(rows *sqlx.Rows) error { return nil }, &nop.Logger{})
	return err
}

func (tctx *testContext) restoreValidResources() error {
	if !tctx.MetaDBCtx.ValidResourcesShrunk {
		panic("valid resources not shrunk yet")
	}
	if _, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryDeleteValidResources, map[string]interface{}{}, func(rows *sqlx.Rows) error { return nil }, &nop.Logger{}); err != nil {
		return err
	}
	if _, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryRestoreValidResources, map[string]interface{}{}, func(rows *sqlx.Rows) error { return nil }, &nop.Logger{}); err != nil {
		return err
	}
	if _, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryDropSavedValidResources, map[string]interface{}{}, func(rows *sqlx.Rows) error { return nil }, &nop.Logger{}); err != nil {
		return err
	}
	tctx.MetaDBCtx.ValidResourcesShrunk = false
	return nil
}

func (tctx *testContext) weEnableFFlagToValidResources(featureFlag string, body *gherkin.DocString) error {
	var condition consolemodels.ResourcePreset
	if err := json.Unmarshal([]byte(body.Content), &condition); err != nil {
		return xerrors.Errorf("json unmarshal error for %q: %w", body.Content, err)
	}
	if _, err := sqlutil.QueryNode(
		tctx.TC.Context(),
		tctx.MetaDB.Node,
		queryAddFFlagValidResources,
		map[string]interface{}{
			"geo_ext_id":   condition.Zone,
			"feature_flag": featureFlag,
		},
		sqlutil.NopParser,
		&nop.Logger{}); err != nil {
		return err
	}
	return nil
}

func (tctx *testContext) weShrinkValidResourcesByParam(shrinkParam, shrinkValue string) error {
	if !tctx.MetaDBCtx.ValidResourcesShrunk {
		if err := tctx.saveValidResources(); err != nil {
			return err
		}
		tctx.MetaDBCtx.ValidResourcesShrunk = true
	}

	statement, ok := validResourcesShrink[shrinkParam]
	if !ok {
		return xerrors.Errorf("unknown shrink param %s", shrinkParam)
	}
	var shrinkPG pgtype.TextArray
	if err := shrinkPG.Set(strings.Split(shrinkValue, ",")); err != nil {
		return err
	}
	if _, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, statement, map[string]interface{}{"shrink_value": shrinkPG}, sqlutil.NopParser, &nop.Logger{}); err != nil {
		return err
	}
	return nil
}

type eventData struct {
	MetaData struct {
		Type string `json:"event_type"`
	} `json:"event_metadata"`
}

func (tctx *testContext) loadEventForTask(taskID string) (string, eventData, error) {
	var data string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&data)
	}

	count, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, querySelectWorkerQueueEvents, map[string]interface{}{"task_id": taskID}, parser, tctx.L)
	if err != nil {
		return "", eventData{}, err
	}

	if count == 0 {
		return "", eventData{}, xerrors.New("expected worker queue event but got none")
	}

	var ed eventData
	if err := json.Unmarshal([]byte(data), &ed); err != nil {
		return "", eventData{}, xerrors.Errorf("json unmarshal error for even data %q: %w", data, err)
	}

	return data, ed, nil
}

func (tctx *testContext) changeFqdnToAndGeoTo(oldFQDN, newFQDN, newGeo string) error {
	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryChangeFQDNAndGeo, map[string]interface{}{"old_fqdn": oldFQDN, "new_fqdn": newFQDN, "new_geo": newGeo}, sqlutil.NopParser, &nop.Logger{})
	return err
}

func (tctx *testContext) allRevsCommittedBefore(cid, ts string) error {
	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryAllRevsCommttedBefore, map[string]interface{}{"cid": cid, "ts": ts}, sqlutil.NopParser, &nop.Logger{})
	return err
}

func (tctx *testContext) lastDocumentInSearchQueueIs(body *gherkin.DocString) error {
	return tctx.checkDocumentInSearchQueueIs(querySelectLastDoc, nil, body)
}

func (tctx *testContext) lastDocumentInSearchQueueMatches(body *gherkin.DocString) error {
	return tctx.checkDocumentInSearchQueueSatisfyMatcher(querySelectLastDoc, nil, body, diff.OptionalKeys)
}

func (tctx *testContext) documentInSearchQueueIs(num string, body *gherkin.DocString) error {
	i, err := strconv.ParseInt(num, 10, 64)
	if err != nil {
		return xerrors.Errorf("failed to parse document number: %w", err)
	}

	return tctx.checkDocumentInSearchQueueIs(querySelectDocByNum, map[string]interface{}{"num": i - 1}, body)
}

func (tctx *testContext) checkDocumentInSearchQueueIs(query sqlutil.Stmt, args map[string]interface{}, body *gherkin.DocString) error {
	return tctx.checkDocumentInSearchQueueSatisfyMatcher(query, args, body, func(expected map[string]interface{}, actual map[string]interface{}) error {
		return diff.Full(expected, actual)
	})
}

func (tctx *testContext) checkDocumentInSearchQueueSatisfyMatcher(query sqlutil.Stmt, args map[string]interface{}, body *gherkin.DocString, matcher func(expected map[string]interface{}, actual map[string]interface{}) error) error {
	// Unmarshal expected to map
	var expected map[string]interface{}
	if err := json.Unmarshal([]byte(body.Content), &expected); err != nil {
		return xerrors.Errorf("failed to unmarshal expected value %s: %w", body.Content, err)
	}

	var doc string
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&doc)
	}
	count, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, query, args, parser, &nop.Logger{})
	if err != nil {
		return err
	}
	if count == 0 {
		return xerrors.New("expected document but got none")
	}

	// Unmarshal actual to map
	var actual map[string]interface{}
	if err = json.Unmarshal([]byte(doc), &actual); err != nil {
		return xerrors.Errorf("failed to unmarshal actual value %s: %w", doc, err)
	}

	for k, v := range expected {
		if v != "<TIMESTAMP>" {
			continue
		}

		if _, ok := actual[k]; !ok {
			return xerrors.Errorf("timestamp key %q is expected but not found in %+v", k, actual)
		}

		delete(expected, k)
		delete(actual, k)
	}

	return matcher(expected, actual)
}

func (tctx *testContext) inSearchQueueThereIsOneDocument() error {
	return tctx.inSearchQueueThereAreDocuments("1")
}

func (tctx *testContext) inSearchQueueThereAreDocuments(arg string) error {
	var actual int64
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&actual)
	}
	count, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, querySearchQueueSize, nil, parser, tctx.L)
	if err != nil {
		return err
	}

	if count == 0 {
		return xerrors.New("expected document count but got none")
	}

	expected, err := strconv.ParseInt(arg, 10, 64)
	if err != nil {
		return xerrors.Errorf("failed to parse expected value %q: %w", arg, err)
	}

	if actual != expected {
		return xerrors.Errorf("expected %d documents but has %d", expected, actual)
	}

	return nil
}

func (tctx *testContext) clusterHasHosts(cid string, hostsNumber string) error {
	expected, _ := strconv.Atoi(hostsNumber)

	var actual int
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&actual)
	}
	count, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, querySelectCountClusterHosts,
		map[string]interface{}{"cid": cid}, parser, &nop.Logger{})
	if err != nil {
		return err
	}

	if count == 0 {
		return xerrors.New("expected document count but got none")
	}

	if actual != expected {
		return xerrors.Errorf("expected %d documents but has %d", expected, actual)
	}

	return diff.Full(expected, actual)
}

func (tctx *testContext) weAddDefaultFeatureFlag(featureFlag string) error {
	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, insertDefaultFeatureFlag, map[string]interface{}{"feature_flag": featureFlag}, sqlutil.NopParser, &nop.Logger{})
	return err
}

func (tctx *testContext) weAddFeatureFlagForCloud(featureFlag string, cloudExtID string) error {
	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryInsertCloudFeatureFlag, map[string]interface{}{"cloud_ext_id": cloudExtID, "feature_flag": featureFlag}, sqlutil.NopParser, tctx.L)
	return err
}

func (tctx *testContext) weAddCloud(cloudExtID string) error {
	params := map[string]interface{}{
		"cloud_ext_id": cloudExtID,
		"cpu":          tctx.InternalAPI.Config.Logic.DefaultCloudQuota.CPU,
		"memory":       tctx.InternalAPI.Config.Logic.DefaultCloudQuota.Memory,
		"ssd":          tctx.InternalAPI.Config.Logic.DefaultCloudQuota.SSDSpace,
		"hdd":          tctx.InternalAPI.Config.Logic.DefaultCloudQuota.HDDSpace,
		"clusters":     tctx.InternalAPI.Config.Logic.DefaultCloudQuota.Clusters,
	}
	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryAddCloud, params, sqlutil.NopParser, &nop.Logger{})
	return err
}

func (tctx *testContext) weAddFolderToCloud(folderExtID string, cloudExtID string) error {
	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryAddFolder, map[string]interface{}{"cloud_ext_id": cloudExtID, "folder_ext_id": folderExtID}, sqlutil.NopParser, &nop.Logger{})
	return err
}

type AddMangedBackupArgs struct {
	BackupID     string          `json:"backup_id"`
	Status       string          `json:"status"`
	Method       string          `json:"method"`
	Initiator    string          `json:"initiator"`
	ClusterID    string          `json:"cid"`
	SubclusterID string          `json:"subcid"`
	ShardID      string          `json:"shard_id"`
	TS           string          `json:"ts"`
	TSStart      string          `json:"start_ts"`
	TSStop       string          `json:"stop_ts"`
	Metadata     json.RawMessage `json:"meta"`
}

func (tctx *testContext) weAddManagedBackup(body *gherkin.DocString) error {
	args := &AddMangedBackupArgs{}
	if err := json.Unmarshal([]byte(body.Content), args); err != nil {
		return fmt.Errorf("can not unmarhall docstring %q with error: %w", body.Content, err)
	}
	params := map[string]interface{}{
		"backup_id":      args.BackupID,
		"status":         args.Status,
		"cid":            args.ClusterID,
		"subcid":         args.SubclusterID,
		"shard_id":       nil,
		"metadata":       string(args.Metadata),
		"method":         args.Method,
		"initiator":      args.Initiator,
		"scheduled_date": nil,
		"ts":             args.TS,
	}
	if args.ShardID != "" {
		params["shard_id"] = args.ShardID
	}

	if args.TSStop == "" {
		params["stop_ts"] = args.TS
	} else {
		params["stop_ts"] = args.TSStop
	}

	if args.TSStart == "" {
		params["start_ts"] = args.TS
	} else {
		params["start_ts"] = args.TSStart
	}

	if args.Initiator == "SCHEDULE" {
		var err error
		params["scheduled_date"], err = time.Parse(time.RFC3339, args.TS)
		if err != nil {
			return fmt.Errorf("can not parse timestamp %q: %w", args.TS, err)
		}
	}

	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryAddManagedBackup, params, sqlutil.NopParser, &nop.Logger{})
	return err
}

func (tctx *testContext) inBackupsThereAreJobs(arg string) error {
	var actual int64
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&actual)
	}
	count, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, querySelectBackupsCount, nil, parser, tctx.L)
	if err != nil {
		return err
	}

	if count == 0 {
		return xerrors.New("expected backups count but got none")
	}

	expected, err := strconv.ParseInt(arg, 10, 64)
	if err != nil {
		return xerrors.Errorf("failed to parse expected value %q: %w", arg, err)
	}

	if actual != expected {
		return xerrors.Errorf("expected %d backups but has %d", expected, actual)
	}

	return nil
}

func (tctx *testContext) weSetManagedBackupStatus(backupID string, _ string, status string) error {
	params := map[string]interface{}{
		"backup_id": backupID,
		"status":    status,
	}
	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, querySetManagedBackupStatus, params, sqlutil.NopParser, &nop.Logger{})
	return err
}

func (tctx *testContext) thatLastBackupHaveStatus(cid string, status string) error {
	params := map[string]interface{}{
		"cid": cid,
	}

	var actual string

	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&actual)
	}

	count, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryGetLastManagedBackupStatus, params, parser, &nop.Logger{})
	if err != nil {
		return err
	}

	if count == 0 {
		return xerrors.New("expected backup but got none")
	}

	if actual != status {
		return xerrors.Errorf("expected %s status but has %s", status, actual)
	}

	return nil
}

func (tctx *testContext) thatBackupHaveStatus(backupID string, status string) error {
	params := map[string]interface{}{
		"backup_id": backupID,
	}

	var actual string

	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&actual)
	}

	count, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryGetManagedBackupStatusByID, params, parser, &nop.Logger{})
	if err != nil {
		return err
	}

	if count == 0 {
		return xerrors.New("expected backup but got none")
	}

	if actual != status {
		return xerrors.Errorf("expected %s status but has %s", status, actual)
	}

	return nil
}

func (tctx *testContext) weEnableBackupServiceForCluster(cid string) error {
	params := map[string]interface{}{
		"cid": cid,
	}
	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryEnableBackupService, params, sqlutil.NopParser, &nop.Logger{})
	return err
}

func (tctx *testContext) initialBackupSuccessfullyCreated(cid string) error {
	params := map[string]interface{}{
		"cid": cid,
	}
	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryCreateInitialBackup, params, sqlutil.NopParser, &nop.Logger{})
	return err
}

func (tctx *testContext) workerSetSecurityGroupsOn(sgIDs, cid string) error {
	params := map[string]interface{}{
		"cid":    cid,
		"sg_ids": strings.Split(sgIDs, ","),
	}
	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, querySetSecurityGroups, params, sqlutil.NopParser, &nop.Logger{})
	return err
}

func (tctx *testContext) workerClearSecurityGroupsFor(cid string) error {
	params := map[string]interface{}{
		"cid": cid,
	}
	_, err := sqlutil.QueryNode(tctx.TC.Context(), tctx.MetaDB.Node, queryClearSecurityGroups, params, sqlutil.NopParser, &nop.Logger{})
	return err
}

func (tctx *testContext) weUpdatePillar(cid, pillarPath, value string) error {
	_, err := sqlutil.QueryNode(
		tctx.TC.Context(),
		tctx.MetaDB.Node,
		queryUpdatePillar,
		map[string]interface{}{
			"cid":   cid,
			"path":  pillarPath,
			"value": value,
		},
		sqlutil.NopParser,
		&nop.Logger{},
	)
	return err
}
