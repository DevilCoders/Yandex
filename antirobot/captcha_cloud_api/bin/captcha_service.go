package main

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"strings"
	"time"

	pb "a.yandex-team.ru/antirobot/captcha_cloud_api/proto"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/quota"
	rmgrpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/resourcemanager/v1"
	smartcaptcha_pb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/smartcaptcha/v1"
	audit_events_pb "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events"
	captcha_audit_pb "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events/smartcaptcha"
	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	"a.yandex-team.ru/library/go/core/xerrors"
	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes"
	"github.com/ydb-platform/ydb-go-sdk/v3/table"
	"github.com/ydb-platform/ydb-go-sdk/v3/table/result"
	"github.com/ydb-platform/ydb-go-sdk/v3/table/types"
	"google.golang.org/genproto/googleapis/rpc/errdetails"
	grpc_status "google.golang.org/genproto/googleapis/rpc/status"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/reflect/protoreflect"
)

const HiddenServerKeyPlaceholder = "***hidden***"

type CaptchaServer struct {
	smartcaptcha_pb.UnimplementedCaptchaSettingsServiceServer
	Server
}

func (server *CaptchaServer) getBy(ctx context.Context, fieldKey string, fieldValue string) ([]*smartcaptcha_pb.CaptchaSettings, error) {
	readTx := table.TxControl(table.BeginTx(table.WithOnlineReadOnly()), table.CommitTx())
	var res result.Result
	queryString := fmt.Sprintf(`-- select
		declare $%[2]v as String;

		SELECT captcha_id, cloud_id, folder_id, client_key, server_key, created_at, updated_at, data
		FROM %[1]v
		WHERE %[2]v == $%[2]v
		ORDER BY created_at
	`, "`captcha-settings`", fieldKey)
	err := (*server.YdbConnection).Table().Do(
		ctx,
		func(ctx context.Context, s table.Session) (err error) {
			_, res, err = s.Execute(
				ctx,
				readTx,
				queryString,
				table.NewQueryParameters(
					table.ValueParam("$"+fieldKey, types.StringValue([]byte(fieldValue))),
				),
			)
			return err
		},
		table.WithIdempotent(),
	)
	if err != nil {
		return nil, err
	}
	if res.Err() != nil {
		return nil, res.Err()
	}
	result := []*smartcaptcha_pb.CaptchaSettings{}

	for res.NextResultSet(ctx, "captcha_id", "cloud_id", "folder_id", "client_key", "server_key", "created_at", "updated_at", "data") {
		for res.NextRow() {
			var (
				captchaID string
				cloudID   string
				folderID  string
				clientKey string
				serverKey string
				createdAt time.Time
				updatedAt time.Time
				data      string
			)
			err = res.ScanWithDefaults(&captchaID, &cloudID, &folderID, &clientKey, &serverKey, &createdAt, &updatedAt, &data)
			if err != nil {
				return nil, err
			}

			captcha := &smartcaptcha_pb.CaptchaSettings{
				CaptchaId: captchaID,
				FolderId:  folderID,
				CloudId:   cloudID,
				ClientKey: clientKey,
				ServerKey: serverKey,
				CreatedAt: Time2Proto(createdAt),
				UpdatedAt: Time2Proto(updatedAt),
			}
			err = fillCaptchaDatabaseRecord([]byte(data), captcha)
			if err != nil {
				return nil, err
			}

			result = append(result, captcha)
		}
	}
	return result, nil
}

func (server *CaptchaServer) checkCaptchaSuspend(ctx context.Context, captcha *smartcaptcha_pb.CaptchaSettings) bool {
	if _, err := server.AuthorizeAnon(ctx, cloudauth.ResourceCloud(captcha.CloudId), CaptchaShowPermission); err != nil {
		if e, ok := status.FromError(err); ok {
			if e.Code() == codes.PermissionDenied {
				return true
			}
			server.EventLogger.Error(FileLine(), fmt.Errorf("checkCaptchaSuspend error: %v %v", e.Code(), e.Message()))
		} else {
			server.EventLogger.Error(FileLine(), fmt.Errorf("checkCaptchaSuspend error: %v", err))
		}
	}
	return false
}

func (server *CaptchaServer) getByClientKeyInternal(ctx context.Context, in *smartcaptcha_pb.GetSettingsByClientKeyRequest) (*smartcaptcha_pb.CaptchaSettings, error) {
	// TODO: метод без авторизации
	// нужно проверять, что пришли с антиробота
	if server.Args.TestSleepDelay > 0 {
		time.Sleep(time.Duration(server.Args.TestSleepDelay) * time.Millisecond)
	}

	items, err := server.getBy(ctx, "client_key", in.ClientKey)
	if err != nil {
		return nil, err
	}
	if len(items) == 0 {
		return nil, status.Errorf(codes.NotFound, "Item does not exists")
	}

	captcha := items[0]
	captcha.Suspend = server.checkCaptchaSuspend(ctx, captcha)
	return captcha, nil
}

func (server *CaptchaServer) GetByClientKey(ctx context.Context, in *smartcaptcha_pb.GetSettingsByClientKeyRequest) (*smartcaptcha_pb.CaptchaSettings, error) {
	result, err := server.getByClientKeyInternal(ctx, in)
	server.AccessLogger.Log(&pb.TLogRecord{CaptchaServerGetByClientKeyRecord: &pb.TCaptchaServerGetByClientKeyRecord{Error: ErrorToProto(err), Request: in, Response: result}})
	return result, err
}

func (server *CaptchaServer) getByServerKeyInternal(ctx context.Context, in *smartcaptcha_pb.GetSettingsByServerKeyRequest) (*smartcaptcha_pb.CaptchaSettings, error) {
	// TODO: метод без авторизации
	// нужно проверять, что пришли с антиробота
	items, err := server.getBy(ctx, "server_key", in.ServerKey)
	if err != nil {
		return nil, err
	}
	if len(items) == 0 {
		return nil, status.Errorf(codes.NotFound, "Item does not exists")
	}

	captcha := items[0]
	captcha.Suspend = server.checkCaptchaSuspend(ctx, captcha)
	return captcha, nil
}

func (server *CaptchaServer) GetByServerKey(ctx context.Context, in *smartcaptcha_pb.GetSettingsByServerKeyRequest) (*smartcaptcha_pb.CaptchaSettings, error) {
	result, err := server.getByServerKeyInternal(ctx, in)
	server.AccessLogger.Log(&pb.TLogRecord{CaptchaServerGetByServerKeyRecord: &pb.TCaptchaServerGetByServerKeyRecord{Error: ErrorToProto(err), Request: in, Response: result}})
	return result, err
}

func (server *CaptchaServer) getInternal(ctx context.Context, in *smartcaptcha_pb.GetSettingsRequest) (*smartcaptcha_pb.CaptchaSettings, error) {
	items, err := server.getBy(ctx, "captcha_id", in.CaptchaId)
	if err != nil {
		return nil, err
	}
	if len(items) == 0 {
		return nil, status.Errorf(codes.NotFound, "Item does not exists")
	}
	captcha := items[0]

	if _, err := server.Authorize(ctx, cloudauth.ResourceFolder(captcha.FolderId), CaptchaGetPermission); err != nil {
		return nil, err
	}

	return captcha, nil
}

func (server *CaptchaServer) Get(ctx context.Context, in *smartcaptcha_pb.GetSettingsRequest) (*smartcaptcha_pb.CaptchaSettings, error) {
	result, err := server.getInternal(ctx, in)
	server.AccessLogger.Log(&pb.TLogRecord{CaptchaServerGetRecord: &pb.TCaptchaServerGetRecord{Error: ErrorToProto(err), Request: in, Response: result}})
	return result, err
}

func createNewCaptcha(request *smartcaptcha_pb.CreateCaptchaRequest, cloudID string, captchaID string) (*smartcaptcha_pb.CaptchaSettings, error) {
	clientKey, serverKey := GenerateClientServerKeys()
	createdAt := Time2Proto(time.Now().UTC())

	return &smartcaptcha_pb.CaptchaSettings{
		CaptchaId:    captchaID,
		FolderId:     request.FolderId,
		CloudId:      cloudID,
		ClientKey:    clientKey,
		ServerKey:    serverKey,
		CreatedAt:    createdAt,
		UpdatedAt:    createdAt,
		Name:         request.Name,
		AllowedSites: request.AllowedSites,
		Complexity:   request.Complexity,
		StyleJson:    request.StyleJson,
	}, nil
}

func makeCaptchaDatabaseRecord(captcha *smartcaptcha_pb.CaptchaSettings) ([]byte, error) {
	dbEecord := &pb.CaptchaSettingsDatabaseRecord{
		Name:           captcha.Name,
		AllowedSites:   captcha.AllowedSites,
		Complexity:     captcha.Complexity,
		StyleJson:      captcha.StyleJson,
		IsYandexClient: captcha.IsYandexClient,
	}
	out, err := proto.Marshal(dbEecord)
	return out, err
}

func makeOperationDatabaseRecord(operation *operation.Operation) ([]byte, error) {
	out, err := proto.Marshal(operation)
	return out, err
}

func fillCaptchaDatabaseRecord(data []byte, captcha *smartcaptcha_pb.CaptchaSettings) error {
	rec := &pb.CaptchaSettingsDatabaseRecord{}
	err := proto.Unmarshal(data, rec)
	if err != nil {
		return err
	}
	captcha.Name = rec.Name
	captcha.AllowedSites = rec.AllowedSites
	captcha.Complexity = rec.Complexity
	captcha.StyleJson = rec.StyleJson
	captcha.IsYandexClient = rec.IsYandexClient
	return nil
}

func wrapOperation(captcha *smartcaptcha_pb.CaptchaSettings, operationType OperationType, operationID string, createdBy string) (*operation.Operation, error) {
	now := Time2Proto(time.Now().UTC())
	var (
		metadata    proto.Message
		description string
	)
	switch operationType {
	case CreateOperation:
		description = "Create captcha"
		metadata = &smartcaptcha_pb.CreateCaptchaMetadata{CaptchaId: captcha.CaptchaId}
	case DeleteOperation:
		description = "Delete captcha"
		metadata = &smartcaptcha_pb.DeleteCaptchaMetadata{CaptchaId: captcha.CaptchaId}
	case UpdateOperation:
		description = "Update captcha"
		metadata = &smartcaptcha_pb.UpdateCaptchaMetadata{CaptchaId: captcha.CaptchaId}
	}

	captchaAny, marshalErr := ptypes.MarshalAny(captcha)
	if marshalErr != nil {
		return nil, marshalErr
	}
	metadataAny, marshalErr := ptypes.MarshalAny(metadata)
	if marshalErr != nil {
		return nil, marshalErr
	}

	return &operation.Operation{
		Id:          fmt.Sprintf("%v", operationID),
		Description: description,
		CreatedAt:   now,
		CreatedBy:   createdBy,
		ModifiedAt:  now,
		Done:        true,
		Metadata:    metadataAny,
		Result:      &operation.Operation_Response{Response: captchaAny},
	}, nil
}

func createQuotaViolationError(metric QuotaMetricName, cloudID string, limit int64, usage int64, required int64) error {
	violation := &quota.QuotaFailure_Violation{
		Metric: &quota.QuotaMetric{
			Name:  string(metric),
			Limit: limit,
			Usage: float64(usage),
		},
		Required: required,
	}

	retErr := status.New(codes.ResourceExhausted, "Quota exceeded")
	retErrDetailed, err := retErr.WithDetails(&quota.QuotaFailure{
		CloudId:    cloudID,
		Violations: []*quota.QuotaFailure_Violation{violation},
	})
	if err != nil {
		return err
	}
	return retErrDetailed.Err()
}

func (server *CaptchaServer) resolveFolder(ctx context.Context, folderID string) (cloudID string, err error) {
	if _, outgoingOK := metadata.FromOutgoingContext(ctx); !outgoingOK {
		incomingMeta, incomingOK := metadata.FromIncomingContext(ctx)
		if !incomingOK {
			return "", xerrors.Errorf("Not incoming context")
		}
		ctx = metadata.NewOutgoingContext(ctx, incomingMeta)
	}

	req := &rmgrpc.ResolveFoldersRequest{
		FolderIds: []string{folderID},
	}

	var resp *rmgrpc.ResolveFoldersResponse
	if resp, err = server.ResourceManagerClient.Resolve(ctx, req); err != nil {
		return "", xerrors.Errorf("Resource manager request failed: %w", err)
	}

	if len(resp.ResolvedFolders) == 0 {
		return "", xerrors.Errorf("folder_id %s is not a folder", folderID)
	}

	return resp.ResolvedFolders[0].CloudId, nil
}

func (server *CaptchaServer) checkSize(captcha *smartcaptcha_pb.CaptchaSettings) error {
	if int64(len(captcha.StyleJson)) > server.Args.StyleJsonMaxSize {
		return status.Errorf(codes.InvalidArgument, "StyleJson is too large")
	}

	var allowedSitesSize int64 = 0
	for _, site := range captcha.AllowedSites {
		allowedSitesSize += int64(len(site))
	}

	if allowedSitesSize > server.Args.AllowedSitesMaxSize {
		return status.Errorf(codes.InvalidArgument, "Sites list is too large")
	}
	return nil
}

func (server *CaptchaServer) Upsert(ctx context.Context, newCaptcha *smartcaptcha_pb.CaptchaSettings, operationType OperationType, subject *Subject) (*operation.Operation, error) {
	if sizeErr := server.checkSize(newCaptcha); sizeErr != nil {
		return nil, sizeErr
	}

	var (
		res                 result.Result
		err                 error
		quotaViolationError error
	)
	captchaDatabaseRecord, errMakeRecord := makeCaptchaDatabaseRecord(newCaptcha)
	if errMakeRecord != nil {
		return nil, errMakeRecord
	}

	resultOperation, operationErr := wrapOperation(
		newCaptcha,
		operationType,
		GenerateOperationID(server.Args.CaptchaIDPrefix),
		subject.GetID(),
	)
	if operationErr != nil {
		return nil, operationErr
	}

	operationDatabaseRecord, errMakeDatabaseRecord := makeOperationDatabaseRecord(resultOperation)
	if errMakeDatabaseRecord != nil {
		return nil, errMakeDatabaseRecord
	}

	err = (*server.YdbConnection).Table().Do(
		ctx,
		func(ctx context.Context, s table.Session) (err error) {
			tx, errTx := s.BeginTransaction(ctx, table.TxSettings(table.WithSerializableReadWrite()))
			if errTx != nil {
				return errTx
			}
			defer func() {
				_ = tx.Rollback(ctx)
			}()

			res, err = tx.Execute(
				ctx,
				`-- count quota usage
				declare $cloud_id as String;
				declare $metric_name as String;
				declare $default_limit as Int64;

				$cnt = (
					select
						count(*) as cnt,
						cloud_id,
						$metric_name as metric
					from `+"`captcha-settings`"+`
					where cloud_id = $cloud_id
					group by cloud_id
				);
				select
					CAST(S.cnt as Int64) as cnt,
					S.cloud_id as cloud_id,
					Q.metric as metric,
					COALESCE(Q.lim, $default_limit) as lim
				from $cnt as S
				left join `+"`cloud-quotas`"+` as Q
				using(cloud_id, metric)
				`,
				table.NewQueryParameters(
					table.ValueParam("$cloud_id", types.StringValue([]byte(newCaptcha.CloudId))),
					table.ValueParam("$metric_name", types.StringValue([]byte(QuotaMetricCaptchasCount))),
					table.ValueParam("$default_limit", types.Int64Value(server.Args.DefaultCaptchasQuota)),
				),
			)
			if err != nil {
				return err
			}
			if res.Err() != nil {
				return res.Err()
			}

			captchasLimit := server.Args.DefaultCaptchasQuota
			var captchasUsage int64 = 0
			for res.NextResultSet(ctx, "cnt", "cloud_id", "metric", "lim") {
				for res.NextRow() {
					var (
						cnt     int64
						cloidID string
						metric  string
						limit   int64
					)
					err = res.ScanWithDefaults(&cnt, &cloidID, &metric, &limit)
					if err != nil {
						return err
					}
					captchasUsage = cnt
					captchasLimit = limit
				}
			}

			required := captchasUsage
			if operationType == CreateOperation {
				required += 1
			}

			if required > captchasLimit {
				quotaViolationError = createQuotaViolationError(QuotaMetricCaptchasCount, newCaptcha.CloudId, captchasLimit, captchasUsage, required)
				return nil
			}

			res, err = tx.Execute(
				ctx,
				`--
				declare $captcha_id as String;
				declare $cloud_id as String;
				declare $folder_id as String;
				declare $client_key as String;
				declare $server_key as String;
				declare $created_at as Timestamp;
				declare $updated_at as Timestamp;
				declare $data as String;

				upsert into `+"`captcha-settings`"+` (captcha_id,  cloud_id,  folder_id,  client_key,  server_key,  created_at,  updated_at,  data)
												  values ($captcha_id, $cloud_id, $folder_id, $client_key, $server_key, $created_at, $updated_at, $data)
				`,
				table.NewQueryParameters(
					table.ValueParam("$captcha_id", types.StringValue([]byte(newCaptcha.CaptchaId))),
					table.ValueParam("$cloud_id", types.StringValue([]byte(newCaptcha.CloudId))),
					table.ValueParam("$folder_id", types.StringValue([]byte(newCaptcha.FolderId))),
					table.ValueParam("$client_key", types.StringValue([]byte(newCaptcha.ClientKey))),
					table.ValueParam("$server_key", types.StringValue([]byte(newCaptcha.ServerKey))),
					table.ValueParam("$created_at", types.TimestampValueFromTime(newCaptcha.CreatedAt.AsTime())),
					table.ValueParam("$updated_at", types.TimestampValueFromTime(newCaptcha.UpdatedAt.AsTime())),
					table.ValueParam("$data", types.StringValue(captchaDatabaseRecord)),
				),
			)
			if err != nil {
				return err
			}
			if res.Err() != nil {
				return res.Err()
			}

			res, err = tx.Execute(
				ctx,
				`--
				declare $operation_id as String;
				declare $captcha_id as String;
				declare $folder_id as String;
				declare $created_at as Timestamp;
				declare $data as String;				

				upsert into `+"`cloud-operations`"+` (operation_id, captcha_id, folder_id, created_at, data)
								values ($operation_id, $captcha_id, $folder_id, $created_at, $data)
				`,
				table.NewQueryParameters(
					table.ValueParam("$operation_id", types.StringValue([]byte(resultOperation.Id))),
					table.ValueParam("$captcha_id", types.StringValue([]byte(newCaptcha.CaptchaId))),
					table.ValueParam("$folder_id", types.StringValue([]byte(newCaptcha.FolderId))),
					table.ValueParam("$created_at", types.TimestampValueFromTime(resultOperation.CreatedAt.AsTime())),
					table.ValueParam("$data", types.StringValue(operationDatabaseRecord)),
				),
			)
			if err != nil {
				return err
			}

			_, err = tx.CommitTx(ctx)
			return err
		},
		table.WithIdempotent(),
	)
	if quotaViolationError != nil {
		return nil, quotaViolationError
	}
	if err != nil {
		return nil, err
	}
	if res.Err() != nil {
		return nil, res.Err()
	}
	return resultOperation, nil
}

func (server *CaptchaServer) logSearchEvent(captcha *smartcaptcha_pb.CaptchaSettings, operationType OperationType) {
	// schema: https://a.yandex-team.ru/svn/trunk/arcadia/cloud/search/schemas/v2/event.json
	timestamp := time.Now().Format(time.RFC3339)
	record := SearchTopicRecord{
		ResourceType: "captcha",
		Timestamp:    timestamp,
		ResourceID:   captcha.CaptchaId,
		Name:         captcha.Name,
		Service:      "smart-captcha",
		Permission:   "smart-captcha.captchas.get",
		CloudID:      captcha.CloudId,
		FolderID:     captcha.FolderId,
		ResourcePath: []ResourcePathRecord{
			{ResourceType: "resource-manager.cloud", ResourceID: captcha.CloudId},
			{ResourceType: "resource-manager.folder", ResourceID: captcha.FolderId},
			{ResourceType: "smart-captcha.captcha", ResourceID: captcha.CaptchaId},
		},
	}
	if operationType == DeleteOperation {
		record.Deleted = timestamp
	}
	jsonBytes, err := json.Marshal(record)
	if err != nil {
		log.Println("Failed to marshal search event JSON:", err)
		return
	}
	server.SearchTopicLogger.LogBytes(jsonBytes)
}

func getCaptchaAuditDetails(captcha *smartcaptcha_pb.CaptchaSettings) *captcha_audit_pb.CaptchaDetails {
	return &captcha_audit_pb.CaptchaDetails{
		CaptchaId:    captcha.CaptchaId,
		ClientKey:    captcha.ClientKey,
		ServerKey:    HiddenServerKeyPlaceholder,
		Name:         captcha.Name,
		AllowedSites: captcha.AllowedSites,
		Complexity:   captcha_audit_pb.CaptchaComplexity(captcha.Complexity),
		StyleJson:    captcha.StyleJson,
	}
}

func (server *CaptchaServer) createInternal(ctx context.Context, in *smartcaptcha_pb.CreateCaptchaRequest, auditEvent *captcha_audit_pb.CreateCaptcha) (*operation.Operation, error) {
	subject, err := server.Authenticate(ctx)
	if err != nil {
		return nil, err
	}
	auditEvent.Authentication = subject.ToAuditAuthentication()

	_, err = server.Authorize(ctx, cloudauth.ResourceFolder(in.FolderId), CaptchaCreatePermission)
	auditEvent.Authorization, auditEvent.Error = authorizeResultToAuditRecord(err, in.FolderId, CaptchaCreatePermission)

	if err != nil {
		return nil, err
	}

	cloudID, err := server.resolveFolder(ctx, in.FolderId)
	if err != nil {
		return nil, err
	}
	auditEvent.EventMetadata.CloudId = cloudID

	newCaptcha, err := createNewCaptcha(in, cloudID, GenerateCaptchaID(server.Args.CaptchaIDPrefix))
	if err != nil {
		return nil, err
	}
	server.EventLogger.Message("Create test message")
	res, err := server.Upsert(ctx, newCaptcha, CreateOperation, subject)
	if err != nil {
		return nil, err
	}
	server.logSearchEvent(newCaptcha, CreateOperation)

	auditEvent.Details = getCaptchaAuditDetails(newCaptcha)
	auditEvent.Response.OperationId = res.Id
	auditEvent.EventStatus = audit_events_pb.EventStatus_DONE
	return res, nil
}

func getRequestMetadata(ctx context.Context) *audit_events_pb.RequestMetadata {
	res := &audit_events_pb.RequestMetadata{
		RemoteAddress: "::1",
	}
	headers, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		return res
	}
	separator := ","
	res.UserAgent = strings.Join(headers["user-agent"], separator)
	res.IdempotencyId = strings.Join(headers["idempotency-key"], separator)
	if requestID, ok := headers["x-request-id"]; ok {
		res.RequestId = strings.Join(requestID, separator)
	} else {
		if requestID, err := GenerateUUID(); err == nil {
			res.RequestId = "x-" + requestID
		}
	}
	if userAddr, ok := headers["x-forwarded-for"]; ok && len(userAddr) > 0 {
		res.RemoteAddress = userAddr[0]
	}

	return res
}

func (server *CaptchaServer) Create(ctx context.Context, in *smartcaptcha_pb.CreateCaptchaRequest) (*operation.Operation, error) {
	auditEvent := &captcha_audit_pb.CreateCaptcha{
		Authentication: &audit_events_pb.Authentication{},
		Authorization:  &audit_events_pb.Authorization{},
		EventMetadata: &audit_events_pb.EventMetadata{
			EventId:   GenerateUUIDSafe(),
			EventType: "yandex.cloud.events.smartcaptcha.CreateCaptcha",
			CreatedAt: Time2Proto(time.Now().UTC()),
			FolderId:  in.FolderId,
		},
		RequestMetadata: getRequestMetadata(ctx),
		EventStatus:     audit_events_pb.EventStatus_ERROR,
		Details:         &captcha_audit_pb.CaptchaDetails{},
		RequestParameters: &captcha_audit_pb.CreateCaptcha_RequestParameters{
			FolderId:     in.FolderId,
			Name:         in.Name,
			AllowedSites: in.AllowedSites,
			Complexity:   captcha_audit_pb.CaptchaComplexity(in.Complexity),
			StyleJson:    in.StyleJson,
		},
		Response: &audit_events_pb.Response{
			OperationId: "",
		},
	}

	result, err := server.createInternal(ctx, in, auditEvent)
	if auditEvent.Error == nil {
		auditEvent.Error = ErrorToGrpcStatus(err)
	}
	server.AccessLogger.Log(&pb.TLogRecord{CaptchaServerCreateRecord: &pb.TCaptchaServerCreateRecord{Error: ErrorToProto(err), Request: in, Response: result}})

	server.AuditLogger.LogAuditEvent(auditEvent)

	return result, err
}

func (server *CaptchaServer) deleteInternal(ctx context.Context, in *smartcaptcha_pb.DeleteCaptchaRequest, auditEvent *captcha_audit_pb.DeleteCaptcha) (*operation.Operation, error) {
	subject, err := server.Authenticate(ctx)
	if err != nil {
		return nil, err
	}
	auditEvent.Authentication = subject.ToAuditAuthentication()

	// TODO: сделать под единой транзакцией

	items, err := server.getBy(ctx, "captcha_id", in.CaptchaId)
	if err != nil {
		return nil, err
	}
	if len(items) == 0 {
		return nil, status.Errorf(codes.NotFound, "Item does not exists")
	}
	captcha := items[0]

	auditEvent.Details = getCaptchaAuditDetails(captcha)

	_, err = server.Authorize(ctx, cloudauth.ResourceFolder(captcha.FolderId), CaptchaDeletePermission)
	auditEvent.Authorization, auditEvent.Error = authorizeResultToAuditRecord(err, captcha.FolderId, CaptchaDeletePermission)
	if err != nil {
		return nil, err
	}

	auditEvent.EventMetadata.CloudId = captcha.CloudId
	auditEvent.EventMetadata.FolderId = captcha.FolderId

	captcha.UpdatedAt = Time2Proto(time.Now().UTC())

	resultOperation, operationErr := wrapOperation(
		captcha,
		DeleteOperation,
		GenerateOperationID(server.Args.CaptchaIDPrefix),
		subject.GetID(),
	)
	if operationErr != nil {
		return nil, operationErr
	}

	operationDatabaseRecord, errMakeDatabaseRecord := makeOperationDatabaseRecord(resultOperation)
	if errMakeDatabaseRecord != nil {
		return nil, errMakeDatabaseRecord
	}

	var res result.Result
	err = (*server.YdbConnection).Table().Do(
		ctx,
		func(ctx context.Context, s table.Session) (err error) {
			tx, errTx := s.BeginTransaction(ctx, table.TxSettings(table.WithSerializableReadWrite()))
			if errTx != nil {
				return errTx
			}
			defer func() {
				_ = tx.Rollback(ctx)
			}()

			res, err = tx.Execute(
				ctx,
				`-- Delete captcha
				declare $captcha_id as String;

				delete from `+"`captcha-settings`"+` where captcha_id=$captcha_id
				`,
				table.NewQueryParameters(
					table.ValueParam("$captcha_id", types.StringValue([]byte(captcha.CaptchaId))),
				),
			)
			if err != nil {
				return err
			}
			if res.Err() != nil {
				return res.Err()
			}

			res, err = tx.Execute(
				ctx,
				`--
				declare $operation_id as String;
				declare $captcha_id as String;
				declare $folder_id as String;
				declare $created_at as Timestamp;
				declare $data as String;				

				upsert into `+"`cloud-operations`"+` (operation_id, captcha_id, folder_id, created_at, data)
								values ($operation_id, $captcha_id, $folder_id, $created_at, $data)
				`,
				table.NewQueryParameters(
					table.ValueParam("$operation_id", types.StringValue([]byte(resultOperation.Id))),
					table.ValueParam("$captcha_id", types.StringValue([]byte(captcha.CaptchaId))),
					table.ValueParam("$folder_id", types.StringValue([]byte(captcha.FolderId))),
					table.ValueParam("$created_at", types.TimestampValueFromTime(resultOperation.CreatedAt.AsTime())),
					table.ValueParam("$data", types.StringValue(operationDatabaseRecord)),
				),
			)
			if err != nil {
				return err
			}

			_, err = tx.CommitTx(ctx)
			return err
		},
		table.WithIdempotent(),
	)
	if err != nil {
		return nil, err
	}
	if res.Err() != nil {
		return nil, res.Err()
	}

	server.logSearchEvent(captcha, DeleteOperation)

	auditEvent.Details = getCaptchaAuditDetails(captcha)
	auditEvent.Response.OperationId = resultOperation.Id
	auditEvent.EventStatus = audit_events_pb.EventStatus_DONE

	return resultOperation, nil
}

func (server *CaptchaServer) Delete(ctx context.Context, in *smartcaptcha_pb.DeleteCaptchaRequest) (*operation.Operation, error) {
	auditEvent := &captcha_audit_pb.DeleteCaptcha{
		Authentication: &audit_events_pb.Authentication{},
		Authorization:  &audit_events_pb.Authorization{},
		EventMetadata: &audit_events_pb.EventMetadata{
			EventId:   GenerateUUIDSafe(),
			EventType: "yandex.cloud.events.smartcaptcha.DeleteCaptcha",
			CreatedAt: Time2Proto(time.Now().UTC()),
		},
		RequestMetadata: getRequestMetadata(ctx),
		EventStatus:     audit_events_pb.EventStatus_ERROR,
		Details:         &captcha_audit_pb.CaptchaDetails{},
		RequestParameters: &captcha_audit_pb.DeleteCaptcha_RequestParameters{
			CaptchaId: in.CaptchaId,
		},
		Response: &audit_events_pb.Response{
			OperationId: "",
		},
	}

	result, err := server.deleteInternal(ctx, in, auditEvent)
	if auditEvent.Error == nil {
		auditEvent.Error = ErrorToGrpcStatus(err)
	}
	server.AccessLogger.Log(&pb.TLogRecord{CaptchaServerDeleteRecord: &pb.TCaptchaServerDeleteRecord{Error: ErrorToProto(err), Request: in, Response: result}})
	server.AuditLogger.LogAuditEvent(auditEvent)
	return result, err
}

func (server *CaptchaServer) listInternal(ctx context.Context, in *smartcaptcha_pb.ListCaptchasRequest) (*smartcaptcha_pb.ListCaptchasResponse, error) {
	if _, err := server.Authorize(ctx, cloudauth.ResourceFolder(in.FolderId), CaptchaGetPermission); err != nil {
		return nil, err
	}

	items, err := server.getBy(ctx, "folder_id", in.FolderId)
	if err != nil {
		return nil, err
	}
	return &smartcaptcha_pb.ListCaptchasResponse{
		Captchas:      items,
		NextPageToken: "",
	}, nil
}

func (server *CaptchaServer) List(ctx context.Context, in *smartcaptcha_pb.ListCaptchasRequest) (*smartcaptcha_pb.ListCaptchasResponse, error) {
	result, err := server.listInternal(ctx, in)
	server.AccessLogger.Log(&pb.TLogRecord{CaptchaServerListRecord: &pb.TCaptchaServerListRecord{Error: ErrorToProto(err), Request: in, Response: result}})
	return result, err
}

func authorizeResultToAuditRecord(err error, folderID string, permission string) (*audit_events_pb.Authorization, *grpc_status.Status) {
	if err != nil {
		// требование вернуть PermissionDenied без деталей от IAM
		retErrDetailed, err2 := status.New(codes.PermissionDenied, "Permission denied").WithDetails(&errdetails.LocalizedMessage{
			Locale:  "en",
			Message: "Permission denied",
		})
		var returnErr *grpc_status.Status = nil
		if err2 == nil {
			returnErr = retErrDetailed.Proto()
		}

		return &audit_events_pb.Authorization{
			Authorized: false,
			Permissions: []*audit_events_pb.RequestedPermissions{
				{
					Permission:   permission,
					ResourceType: "resource-manager.folder",
					ResourceId:   folderID,
					Authorized:   false,
				},
			},
		}, returnErr
	}
	return &audit_events_pb.Authorization{
		Authorized: true,
		Permissions: []*audit_events_pb.RequestedPermissions{
			{
				Permission:   permission,
				ResourceType: "resource-manager.folder",
				ResourceId:   folderID,
				Authorized:   true,
			},
		},
	}, nil
}

func (server *CaptchaServer) updateInternal(ctx context.Context, in *smartcaptcha_pb.UpdateCaptchaRequest, auditEvent *captcha_audit_pb.UpdateCaptcha) (*operation.Operation, error) {
	subject, err := server.Authenticate(ctx)
	if err != nil {
		return nil, err
	}
	auditEvent.Authentication = subject.ToAuditAuthentication()

	// TODO: сделать под единой транзакцией
	// NOTE: сначала достаём из базы капчу, а потом проверяем если ли на неё права

	items, err := server.getBy(ctx, "captcha_id", in.CaptchaId)
	if err != nil {
		return nil, err
	}
	if len(items) == 0 {
		return nil, status.Errorf(codes.NotFound, "Item does not exists")
	}
	captcha := items[0]

	auditEvent.Details = getCaptchaAuditDetails(captcha)

	_, err = server.Authorize(ctx, cloudauth.ResourceFolder(captcha.FolderId), CaptchaUpdatePermission)
	auditEvent.Authorization, auditEvent.Error = authorizeResultToAuditRecord(err, captcha.FolderId, CaptchaUpdatePermission)

	if err != nil {
		return nil, err
	}

	auditEvent.EventMetadata.CloudId = captcha.CloudId
	auditEvent.EventMetadata.FolderId = captcha.FolderId

	captcha.UpdatedAt = Time2Proto(time.Now().UTC())
	for _, path := range in.UpdateMask.GetPaths() {
		switch path {
		case "name":
			captcha.Name = in.Name
		case "allowed_sites", "allowedSites":
			captcha.AllowedSites = in.AllowedSites
		case "complexity":
			captcha.Complexity = in.Complexity
		case "style_json", "styleJson":
			captcha.StyleJson = in.StyleJson
		default:
			return nil, status.Errorf(codes.InvalidArgument, fmt.Sprintf("Incorrect field '%v'", path))
		}
	}

	res, err := server.Upsert(ctx, captcha, UpdateOperation, subject)
	if err != nil {
		return nil, err
	}
	server.logSearchEvent(captcha, UpdateOperation)

	auditEvent.Details = getCaptchaAuditDetails(captcha)
	auditEvent.Response.OperationId = res.Id
	auditEvent.EventStatus = audit_events_pb.EventStatus_DONE

	return res, nil
}

func (server *CaptchaServer) Update(ctx context.Context, in *smartcaptcha_pb.UpdateCaptchaRequest) (*operation.Operation, error) {
	auditEvent := &captcha_audit_pb.UpdateCaptcha{
		Authentication: &audit_events_pb.Authentication{},
		Authorization:  &audit_events_pb.Authorization{},
		EventMetadata: &audit_events_pb.EventMetadata{
			EventId:   GenerateUUIDSafe(),
			EventType: "yandex.cloud.events.smartcaptcha.UpdateCaptcha",
			CreatedAt: Time2Proto(time.Now().UTC()),
		},
		RequestMetadata: getRequestMetadata(ctx),
		EventStatus:     audit_events_pb.EventStatus_ERROR,
		Details:         &captcha_audit_pb.CaptchaDetails{},
		RequestParameters: &captcha_audit_pb.UpdateCaptcha_RequestParameters{
			CaptchaId:    in.CaptchaId,
			UpdateMask:   in.UpdateMask,
			Name:         in.Name,
			AllowedSites: in.AllowedSites,
			Complexity:   captcha_audit_pb.CaptchaComplexity(in.Complexity),
			StyleJson:    in.StyleJson,
		},
		Response: &audit_events_pb.Response{
			OperationId: "",
		},
	}

	result, err := server.updateInternal(ctx, in, auditEvent)
	if auditEvent.Error == nil {
		auditEvent.Error = ErrorToGrpcStatus(err)
	}
	server.AccessLogger.Log(&pb.TLogRecord{CaptchaServerUpdateRecord: &pb.TCaptchaServerUpdateRecord{Error: ErrorToProto(err), Request: in, Response: result}})
	server.AuditLogger.LogAuditEvent(auditEvent)
	return result, err
}

func (server *CaptchaServer) updateAllInternal(ctx context.Context, in *smartcaptcha_pb.UpdateAllCaptchaRequest, auditEvent *captcha_audit_pb.UpdateCaptcha) (*operation.Operation, error) {
	subject, err := server.Authenticate(ctx)
	if err != nil {
		return nil, err
	}
	auditEvent.Authentication = subject.ToAuditAuthentication()

	// TODO: сделать под единой транзакцией
	// NOTE: сначала достаём из базы капчу, а потом проверяем если ли на неё права

	items, err := server.getBy(ctx, "captcha_id", in.CaptchaId)
	if err != nil {
		return nil, err
	}
	if len(items) == 0 {
		return nil, status.Errorf(codes.NotFound, "Item does not exists")
	}
	captcha := items[0]

	auditEvent.Details = getCaptchaAuditDetails(captcha)

	_, err = server.Authorize(ctx, cloudauth.ResourceFolder(captcha.FolderId), CaptchaUpdateAllPermission)
	auditEvent.Authorization, auditEvent.Error = authorizeResultToAuditRecord(err, captcha.FolderId, CaptchaUpdatePermission)
	if err != nil {
		return nil, err
	}

	auditEvent.EventMetadata.CloudId = captcha.CloudId
	auditEvent.EventMetadata.FolderId = captcha.FolderId

	dstReflection := captcha.ProtoReflect()
	srcReflection := in.Settings.ProtoReflect()

	for _, path := range in.UpdateMask.GetPaths() {
		dstFieldDescriptor := dstReflection.Descriptor().Fields().ByName(protoreflect.Name(path))
		if dstFieldDescriptor == nil {
			return nil, status.Errorf(codes.InvalidArgument, fmt.Sprintf("Incorrect field '%v'", path))
		}
		srcFieldDescriptor := srcReflection.Descriptor().Fields().ByName(protoreflect.Name(path))

		dstReflection.Set(dstFieldDescriptor, srcReflection.Get(srcFieldDescriptor))
	}

	res, err := server.Upsert(ctx, captcha, UpdateOperation, subject)
	if err != nil {
		return nil, err
	}
	server.logSearchEvent(captcha, UpdateOperation)

	auditEvent.Details = getCaptchaAuditDetails(captcha)
	auditEvent.Response.OperationId = res.Id
	auditEvent.EventStatus = audit_events_pb.EventStatus_DONE

	return res, nil
}

func (server *CaptchaServer) UpdateAll(ctx context.Context, in *smartcaptcha_pb.UpdateAllCaptchaRequest) (*operation.Operation, error) {
	auditEvent := &captcha_audit_pb.UpdateCaptcha{
		Authentication: &audit_events_pb.Authentication{},
		Authorization:  &audit_events_pb.Authorization{},
		EventMetadata: &audit_events_pb.EventMetadata{
			EventId:   GenerateUUIDSafe(),
			EventType: "yandex.cloud.events.smartcaptcha.UpdateCaptcha",
			CreatedAt: Time2Proto(time.Now().UTC()),
		},
		RequestMetadata: getRequestMetadata(ctx),
		EventStatus:     audit_events_pb.EventStatus_ERROR,
		Details:         &captcha_audit_pb.CaptchaDetails{},
		RequestParameters: &captcha_audit_pb.UpdateCaptcha_RequestParameters{
			CaptchaId:    in.CaptchaId,
			UpdateMask:   in.UpdateMask,
			Name:         in.Settings.Name,
			AllowedSites: in.Settings.AllowedSites,
			Complexity:   captcha_audit_pb.CaptchaComplexity(in.Settings.Complexity),
			StyleJson:    in.Settings.StyleJson,
			ClientKey:    in.Settings.ClientKey,
			ServerKey:    HiddenServerKeyPlaceholder,
		},
		Response: &audit_events_pb.Response{
			OperationId: "",
		},
	}

	result, err := server.updateAllInternal(ctx, in, auditEvent)
	if auditEvent.Error == nil {
		auditEvent.Error = ErrorToGrpcStatus(err)
	}
	server.AccessLogger.Log(&pb.TLogRecord{CaptchaServerUpdateAllRecord: &pb.TCaptchaServerUpdateAllRecord{Error: ErrorToProto(err), Request: in, Response: result}})
	server.AuditLogger.LogAuditEvent(auditEvent)
	return result, err
}
