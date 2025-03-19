package marketplace

import (
	"context"
	"encoding/json"
	"fmt"
	"time"

	"a.yandex-team.ru/library/go/core/log"
)

type operationManager interface {
	GetOperationByID(operationID string) (*Operation, error)
	WaitOperation(operationID string) (*Operation, error)
}

type Operation struct {
	ID          string `json:"id"`
	PublisherID string `json:"publisherId"`
	Description string `json:"description"`
	CreatedBy   string `json:"createdBy"`
	CreatedAt   string `json:"createdAt"`
	ModifiedAt  string `json:"modifiedAt"`

	Done     bool            `json:"done"`
	Metadata json.RawMessage `json:"metadata"`
	Response json.RawMessage `json:"response"`
	Error    *OperationError `json:"error"`
}

type OperationError struct {
	Prefix  string          `json:"prefix"`
	Code    string          `json:"code"`
	Message string          `json:"message"`
	Details json.RawMessage `json:"details"`
}

func (s *Session) GetOperationByID(operationID string) (*Operation, error) {
	var result Operation

	response, err := s.ctxAuthRequest().
		SetResult(&result).
		SetPathParam("operationID", operationID).
		Get("/marketplace/v2/private/operations/{operationID}")

	if err := s.mapError(s.ctx, response, err); err != nil {
		return nil, err
	}

	return &result, nil
}

func (s *Session) WaitOperation(operationID string) (*Operation, error) {

	ctx, cancel := context.WithTimeout(s.ctx, s.opTimeout)
	defer cancel()

	scopedLogger := log.With(s.logger, log.String("operation_id", operationID))
	for {
		scopedLogger.Debug("polling operation ...")
		op, err := s.GetOperationByID(operationID)
		if err != nil {
			scopedLogger.Debug("operation polling failed")
			return nil, fmt.Errorf("failed to poll operation: %v", err)
		}

		if op.Done {
			if op.Error != nil {
				errDetails, innerErr := json.Marshal(&op.Error.Details)
				if innerErr != nil {
					return nil, fmt.Errorf("error marshalling operation %s details: %v", operationID, innerErr)
				}
				return nil, fmt.Errorf("operation %s failed with code %s: %s %s", operationID, op.Error.Code, op.Error.Message, errDetails)
			}

			scopedLogger.Debug("operation is done")
			return op, nil
		}
		select {
		case <-ctx.Done():
			return nil, fmt.Errorf("timeout waiting for operation: %v", ctx.Err())
		case <-time.After(s.opInterval):
		}
	}
}
