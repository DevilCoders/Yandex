package kmslbclient

import (
	"context"
	"encoding/base64"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/timestamp"
	"google.golang.org/grpc/metadata"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1"
)

type IAMTokenProvider func() string

type AccessKeySignatureV2 struct {
	KeyID           string
	StringToSign    string
	Signature       string
	SignatureMethod servicecontrol.AccessKeySignature_Version2Parameters_SignatureMethod
}

type AccessKeySignatureV4 struct {
	KeyID        string
	StringToSign string
	Signature    string
	Region       string
	SignedAt     time.Time
	Service      string
}

type CallCredentials struct {
	IAMToken    *string
	SignatureV2 *AccessKeySignatureV2
	SignatureV4 *AccessKeySignatureV4
}

func appendAuthorization(ctx context.Context, tokenProvider IAMTokenProvider, callCreds *CallCredentials) (context.Context, error) {
	if callCreds != nil {
		if callCreds.IAMToken != nil {
			return metadata.AppendToOutgoingContext(ctx, "Authorization", "Bearer "+*callCreds.IAMToken), nil
		} else if callCreds.SignatureV2 != nil {
			header, err := serializeSignatureV2(callCreds.SignatureV2)
			if err != nil {
				return nil, err
			}
			return metadata.AppendToOutgoingContext(ctx, "X-YC-AccessKey-Authorization", header), nil
		} else if callCreds.SignatureV4 != nil {
			header, err := serializeSignatureV4(callCreds.SignatureV4)
			if err != nil {
				return nil, err
			}
			return metadata.AppendToOutgoingContext(ctx, "X-YC-AccessKey-Authorization", header), nil
		} else {
			return nil, fmt.Errorf("no credentials set in CallCredentials")
		}
	}
	return metadata.AppendToOutgoingContext(ctx, "Authorization", "Bearer "+tokenProvider()), nil
}

func serializeSignatureV2(signature *AccessKeySignatureV2) (string, error) {
	aks := &servicecontrol.AccessKeySignature{
		AccessKeyId:  signature.KeyID,
		StringToSign: signature.StringToSign,
		Signature:    signature.Signature,
		Parameters: &servicecontrol.AccessKeySignature_V2Parameters{
			V2Parameters: &servicecontrol.AccessKeySignature_Version2Parameters{
				SignatureMethod: signature.SignatureMethod,
			},
		},
	}
	out, err := proto.Marshal(aks)
	if err != nil {
		return "", err
	}
	return base64.StdEncoding.EncodeToString(out), nil
}

func serializeSignatureV4(signature *AccessKeySignatureV4) (string, error) {
	aks := &servicecontrol.AccessKeySignature{
		AccessKeyId:  signature.KeyID,
		StringToSign: signature.StringToSign,
		Signature:    signature.Signature,
		Parameters: &servicecontrol.AccessKeySignature_V4Parameters{
			V4Parameters: &servicecontrol.AccessKeySignature_Version4Parameters{
				SignedAt: &timestamp.Timestamp{Seconds: signature.SignedAt.Unix()},
				Region:   signature.Region,
				Service:  signature.Service,
			},
		},
	}
	out, err := proto.Marshal(aks)
	if err != nil {
		return "", err
	}
	return base64.StdEncoding.EncodeToString(out), nil
}
