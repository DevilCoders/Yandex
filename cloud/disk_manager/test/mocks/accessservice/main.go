package main

import (
	"context"
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"regexp"

	"github.com/golang/protobuf/proto"
	"github.com/spf13/cobra"
	"google.golang.org/grpc"
	grpc_codes "google.golang.org/grpc/codes"
	"google.golang.org/grpc/credentials"
	grpc_status "google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1"
	"a.yandex-team.ru/cloud/disk_manager/test/mocks/accessservice/config"
)

////////////////////////////////////////////////////////////////////////////////

type accessService struct {
	serviceConfig *config.AccessServiceMockConfig
}

func (s *accessService) getSubject(
	iamToken string,
) (*servicecontrol.Subject, error) {

	id, ok := s.serviceConfig.GetIamTokenToUserId()[iamToken]
	if !ok {
		return nil, grpc_status.Errorf(
			grpc_codes.Unauthenticated,
			"No user for token %v",
			iamToken,
		)
	}
	return &servicecontrol.Subject{
		Type: &servicecontrol.Subject_UserAccount_{
			UserAccount: &servicecontrol.Subject_UserAccount{
				Id: id,
			},
		},
	}, nil
}

func (s *accessService) authorizeID(
	id string,
	permission string,
) error {

	for _, rule := range s.serviceConfig.GetRules() {
		matchedID, err := regexp.MatchString(rule.GetIdPattern(), id)
		if err != nil {
			return err
		}

		if !matchedID {
			continue
		}

		matchedPermission, err := regexp.MatchString(
			rule.GetPermissionPattern(),
			permission,
		)
		if err != nil {
			return err
		}

		if matchedPermission {
			return nil
		}
	}
	return grpc_status.Errorf(
		grpc_codes.PermissionDenied,
		"Permission %v denied for %v",
		permission,
		id,
	)
}

func (s *accessService) authorizeSubject(
	subj *servicecontrol.Subject,
	permission string,
) error {
	switch account := subj.Type.(type) {
	case *servicecontrol.Subject_UserAccount_:
		return s.authorizeID(account.UserAccount.Id, permission)
	case *servicecontrol.Subject_ServiceAccount_:
		return s.authorizeID(account.ServiceAccount.Id, permission)
	case *servicecontrol.Subject_AnonymousAccount_:
		return grpc_status.Errorf(
			grpc_codes.PermissionDenied,
			"Cannot authorize anonymous account",
		)
	default:
		return fmt.Errorf("unknown subject type %v", account)
	}
}

func (s *accessService) Authenticate(
	ctx context.Context,
	req *servicecontrol.AuthenticateRequest,
) (*servicecontrol.AuthenticateResponse, error) {
	switch creds := req.Credentials.(type) {
	case *servicecontrol.AuthenticateRequest_IamToken:
		subj, err := s.getSubject(creds.IamToken)
		if err != nil {
			return nil, err
		}
		return &servicecontrol.AuthenticateResponse{
			Subject: subj,
		}, nil
	case *servicecontrol.AuthenticateRequest_Signature:
		return nil, grpc_status.Error(
			grpc_codes.Unimplemented,
			"AccessKeySignature not implemented",
		)
	case *servicecontrol.AuthenticateRequest_ApiKey:
		return nil, grpc_status.Error(
			grpc_codes.Unimplemented,
			"ApiKey not implemented",
		)
	default:
		return nil, grpc_status.Errorf(
			grpc_codes.InvalidArgument,
			"Unknown credentials type %v", creds)
	}
}

func (s *accessService) Authorize(
	ctx context.Context,
	req *servicecontrol.AuthorizeRequest,
) (*servicecontrol.AuthorizeResponse, error) {
	switch ident := req.Identity.(type) {
	case *servicecontrol.AuthorizeRequest_Subject:
		err := s.authorizeSubject(ident.Subject, req.Permission)
		if err != nil {
			return nil, err
		}
		return &servicecontrol.AuthorizeResponse{
			Subject:      ident.Subject,
			ResourcePath: req.ResourcePath,
		}, nil
	case *servicecontrol.AuthorizeRequest_IamToken:
		subj, err := s.getSubject(ident.IamToken)
		if err != nil {
			return nil, err
		}
		err = s.authorizeSubject(subj, req.Permission)
		if err != nil {
			return nil, err
		}
		return &servicecontrol.AuthorizeResponse{
			Subject:      subj,
			ResourcePath: req.ResourcePath,
		}, nil
	case *servicecontrol.AuthorizeRequest_Signature:
		return nil, grpc_status.Error(
			grpc_codes.Unimplemented,
			"AccessKeySignature not implemented",
		)
	case *servicecontrol.AuthorizeRequest_ApiKey:
		return nil, grpc_status.Error(
			grpc_codes.Unimplemented,
			"ApiKey not implemented",
		)
	default:
		return nil, grpc_status.Errorf(
			grpc_codes.InvalidArgument,
			"Unknown identity type %v", ident)
	}
}

func (s *accessService) BulkAuthorize(
	ctx context.Context,
	req *servicecontrol.BulkAuthorizeRequest,
) (*servicecontrol.BulkAuthorizeResponse, error) {
	return nil, grpc_status.Error(
		grpc_codes.Unimplemented,
		"BulkAuthorize not implemented",
	)
}

////////////////////////////////////////////////////////////////////////////////

func run(serviceConfig *config.AccessServiceMockConfig) error {
	creds, err := credentials.NewServerTLSFromFile(
		serviceConfig.GetCertFile(),
		serviceConfig.GetPrivateKeyFile(),
	)
	if err != nil {
		return fmt.Errorf("failed to create creds: %w", err)
	}

	lis, err := net.Listen("tcp", fmt.Sprintf(":%d", serviceConfig.GetPort()))
	if err != nil {
		return fmt.Errorf("failed to listen on %v: %w", serviceConfig.GetPort(), err)
	}

	log.Printf("listening on %v", lis.Addr().String())

	srv := grpc.NewServer(grpc.Creds(creds))
	servicecontrol.RegisterAccessServiceServer(srv, &accessService{
		serviceConfig: serviceConfig,
	})

	return srv.Serve(lis)
}

////////////////////////////////////////////////////////////////////////////////

func parseConfig(
	configFileName string,
	serviceConfig *config.AccessServiceMockConfig,
) error {

	configBytes, err := ioutil.ReadFile(configFileName)
	if err != nil {
		return fmt.Errorf(
			"failed to read config file %v: %w",
			configFileName,
			err,
		)
	}

	err = proto.UnmarshalText(string(configBytes), serviceConfig)
	if err != nil {
		return fmt.Errorf(
			"failed to parse config file %v as protobuf: %w",
			configFileName,
			err,
		)
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func main() {
	log.Println("accessservice-mock launched")

	var configFileName string
	serviceConfig := &config.AccessServiceMockConfig{}

	var rootCmd = &cobra.Command{
		Use:   "accessservice-mock",
		Short: "Mock for IAM's accessservice",
		PersistentPreRunE: func(cmd *cobra.Command, args []string) error {
			return parseConfig(configFileName, serviceConfig)
		},
		Run: func(cmd *cobra.Command, args []string) {
			err := run(serviceConfig)
			if err != nil {
				log.Fatalf("Error: %v", err)
			}
		},
	}
	rootCmd.Flags().StringVar(
		&configFileName,
		"config",
		"accessservice-mock-config.txt",
		"Path to the config file",
	)

	if err := rootCmd.Execute(); err != nil {
		log.Fatalf("Error: %v", err)
	}
}
