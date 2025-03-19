package main

import (
	"context"
	"strings"

	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"

	protos "a.yandex-team.ru/cloud/blockstore/private/api/protos"
	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
)

func executeAction(
	ctx context.Context,
	client *nbs.Client,
	action string,
	request proto.Message,
	response proto.Message) error {

	input, err := new(jsonpb.Marshaler).MarshalToString(request)
	if err != nil {
		return err
	}

	output, err := client.ExecuteAction(ctx, action, []byte(input))
	if err != nil {
		return err
	}

	reader := strings.NewReader(string(output))

	err = new(jsonpb.Unmarshaler).Unmarshal(reader, response)
	if err != nil {
		return err
	}

	return nil
}

func CheckBlob(
	ctx context.Context,
	client *nbs.Client,
	request *protos.TCheckBlobRequest,
) (*protos.TCheckBlobResponse, error) {

	response := &protos.TCheckBlobResponse{}

	err := executeAction(ctx, client, "CheckBlob", request, response)
	if err != nil {
		return nil, err
	}

	return response, nil
}

func ResetTablet(
	ctx context.Context,
	client *nbs.Client,
	request *protos.TResetTabletRequest,
) (*protos.TResetTabletResponse, error) {

	response := &protos.TResetTabletResponse{}

	err := executeAction(ctx, client, "ResetTablet", request, response)
	if err != nil {
		return nil, err
	}

	return response, nil
}

func DescribeBlocks(
	ctx context.Context,
	client *nbs.Client,
	request *protos.TDescribeBlocksRequest,
) (*protos.TDescribeBlocksResponse, error) {

	response := &protos.TDescribeBlocksResponse{}

	err := executeAction(ctx, client, "DescribeBlocks", request, response)
	if err != nil {
		return nil, err
	}

	return response, nil
}

func CompactRange(
	ctx context.Context,
	client *nbs.Client,
	request *protos.TCompactRangeRequest,
) (*protos.TCompactRangeResponse, error) {

	response := &protos.TCompactRangeResponse{}

	err := executeAction(ctx, client, "CompactRange", request, response)
	if err != nil {
		return nil, err
	}

	return response, nil
}
