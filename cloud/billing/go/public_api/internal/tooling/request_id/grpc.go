package requestid

import (
	"context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/metadata"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/logging"
	"a.yandex-team.ru/library/go/core/log"
)

const requestIDMetadataKey string = "x-request-id"

func extractValue(ctx context.Context, key string) string {
	md, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		return ""
	}

	if len(md[key]) == 1 {
		return md[key][0]
	}

	return ""
}

func extractRequestID(ctx context.Context) string {
	return extractValue(ctx, requestIDMetadataKey)
}

func ExtractOrGenerateGRPC(ctx context.Context, logger log.Logger) string {

	originRequestID := extractRequestID(ctx)
	if originRequestID == "" {
		return generateRequestID()
	}

	requestID, err := parseValidUUID(originRequestID)
	if err != nil {
		requestID = generateRequestID()
		logger.Error("incorrect request id", logging.RequestID(requestID), logging.OriginRequestID(originRequestID))
	}

	return requestID
}

func InjectGRPC(ctx context.Context, requestID string) error {
	header := metadata.Pairs(requestIDHeaderName, requestID)
	return grpc.SendHeader(ctx, header)
}
