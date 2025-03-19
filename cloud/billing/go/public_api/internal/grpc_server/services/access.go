package services

import (
	"context"
	"strings"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/access"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
)

func (b BillingAccountService) ListAccessBindings(ctx context.Context, request *access.ListAccessBindingsRequest) (*access.ListAccessBindingsResponse, error) {
	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	consoleReq := &console.ListAccessBindingsRequest{
		BillingAccountID: request.ResourceId,
	}

	if request.PageToken != "" {
		consoleReq.PageToken = &request.PageToken
	}

	if request.PageSize != 0 {
		consoleReq.PageSize = &request.PageSize
	}

	authData := &console.AuthData{Token: token}

	consoleResp, err := b.client.ListAccessBindings(ctx, consoleReq, authData)
	if err != nil {
		return nil, parseError(err)
	}

	response := &access.ListAccessBindingsResponse{
		AccessBindings: mapAccessBindings(consoleResp.AccessBindings),
	}

	if consoleResp.NextPageToken != nil {
		response.NextPageToken = *consoleResp.NextPageToken
	}

	return response, nil
}

func (b BillingAccountService) UpdateAccessBindings(ctx context.Context, request *access.UpdateAccessBindingsRequest) (*operation.Operation, error) {
	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	authData := &console.AuthData{Token: token}
	consoleReq := &console.UpdateAccessBindingsRequest{
		BillingAccountID:     request.ResourceId,
		AccessBindingsDeltas: mapConsoleAccessBindingDeltas(request.AccessBindingDeltas),
	}

	consoleOperation, err := b.client.UpdateAccessBindings(ctx, consoleReq, authData)
	if err != nil {
		return nil, parseError(err)
	}

	return FormatOperation(consoleOperation)
}

func mapConsoleAccessBindingDeltas(deltas []*access.AccessBindingDelta) []console.AccessBindingDelta {
	var res []console.AccessBindingDelta
	for _, delta := range deltas {
		res = append(res, *mapConsoleAccessBindingDelta(delta))
	}

	return res
}

func mapConsoleAccessBindingDelta(delta *access.AccessBindingDelta) *console.AccessBindingDelta {
	if delta == nil {
		return &console.AccessBindingDelta{}
	}

	return &console.AccessBindingDelta{
		Action:        strings.ToLower(delta.Action.String()),
		AccessBinding: *mapConsoleAccessBinding(delta.AccessBinding),
	}
}

func mapAccessBindings(bindings []console.AccessBinding) []*access.AccessBinding {
	var res []*access.AccessBinding
	for _, binding := range bindings {
		res = append(res, mapAccessBinding(binding))
	}

	return res
}

func mapAccessBinding(binding console.AccessBinding) *access.AccessBinding {
	return &access.AccessBinding{
		RoleId:  binding.RoleID,
		Subject: mapSubject(&binding.Subject),
	}
}

func mapConsoleAccessBinding(binding *access.AccessBinding) *console.AccessBinding {
	if binding == nil {
		return &console.AccessBinding{}
	}

	return &console.AccessBinding{
		RoleID:  binding.RoleId,
		Subject: *mapConsoleSubject(binding.Subject),
	}
}

func mapSubject(sbj *console.Subject) *access.Subject {
	return &access.Subject{
		Id:   sbj.ID,
		Type: sbj.Type,
	}
}

func mapConsoleSubject(sbj *access.Subject) *console.Subject {
	if sbj == nil {
		return &console.Subject{}
	}

	return &console.Subject{
		ID:   sbj.Id,
		Type: sbj.Type,
	}
}
