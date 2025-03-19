package grpcmocker

import (
	"context"
	"encoding/json"
	"strings"

	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"
	"github.com/jhump/protoreflect/desc"
	"github.com/jhump/protoreflect/dynamic"
	"github.com/jhump/protoreflect/grpcreflect"
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker/expectations"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Mock wires gRPC server and expectations. Can be used stand-alone without Run/Server part if user has special requirements.
type Mock struct {
	descriptors map[string]*desc.ServiceDescriptor
	matcher     expectations.Matcher
}

// Register expectations
func (m *Mock) Register(s *grpc.Server, matcher expectations.Matcher) error {
	sds, err := grpcreflect.LoadServiceDescriptors(s)
	if err != nil {
		return xerrors.Errorf("load service descriptors: %w", err)
	}

	m.descriptors = sds
	m.matcher = matcher
	return nil
}

// NewResponse returns response instance of correct proto type
func (m *Mock) NewResponse(service, method string) (proto.Message, error) {
	sd, ok := m.descriptors[service]
	if !ok {
		return nil, xerrors.Errorf("service %q not found (method %q)", service, method)
	}

	md := sd.FindMethodByName(method)
	if md == nil {
		return nil, xerrors.Errorf("method %q not found in service %q", method, service)
	}

	return dynamic.NewMessage(md.GetOutputType()), nil
}

// NewUnaryServerInterceptor returns interceptor that maps incoming unary requests to expectations.
func (m *Mock) NewUnaryServerInterceptor(l log.Logger) grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		protoReq := req.(proto.Message)
		l.Debugf("%s request %s", info.FullMethod, protoReq)

		jsonReq := jsonpb.Marshaler{EmitDefaults: false, OrigName: true}
		s, err := jsonReq.MarshalToString(protoReq)
		if err != nil {
			return nil, xerrors.Errorf("marshal request %s: %w", protoReq.String(), err)
		}

		var actual map[string]interface{}
		if err = json.Unmarshal([]byte(s), &actual); err != nil {
			return nil, xerrors.Errorf("unmarshal request %s: %w", s, err)
		}

		location := strings.Split(info.FullMethod, "/")
		matchInputs := expectations.MatchInputs{
			FullMethod:        info.FullMethod,
			Service:           location[1],
			Method:            location[2],
			ProtoRequest:      protoReq,
			StructuredRequest: actual,
		}
		r, err := m.matcher.Match(ctx, matchInputs)
		if err == nil {
			respondInputs := expectations.RespondInputs{
				FullMethod:        matchInputs.FullMethod,
				Service:           matchInputs.Service,
				Method:            matchInputs.Method,
				ProtoRequest:      matchInputs.ProtoRequest,
				StructuredRequest: matchInputs.StructuredRequest,
				NewResponse:       m.NewResponse,
			}
			resp, err := r.Respond(ctx, respondInputs)
			if err != nil && resp != nil {
				l.Debugf("match %s, response %s with error %s", info.FullMethod, resp, err)
			} else if err != nil {
				l.Debugf("match %s, response with error %s", info.FullMethod, err)
			} else {
				l.Debugf("match %s, response %s", info.FullMethod, resp)
			}
			return resp, err
		}

		if !xerrors.Is(err, expectations.ErrNoMatch) {
			l.Debugf("%s match error %s", info.FullMethod, err)
			return nil, err
		}

		resp, err := handler(ctx, req)
		if err != nil && resp != nil {
			l.Debugf("%s default response %s with error %s", info.FullMethod, resp, err)
		} else if err != nil {
			l.Debugf("%s default response with error %s", info.FullMethod, err)
		} else {
			l.Debugf("%s default response %s", info.FullMethod, resp)
		}
		return resp, err
	}
}
