package server

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/dataplatform/api/connman"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/storage"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/storage/types"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/view"
	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/transfer_manager/go/pkg/auth"
)

const (
	errUnableToGetConnection = "unable to get connection: %w"
)

type connectionServer struct {
	storage storage.Storage
	auth    auth.Provider
	viewer  view.Viewer
}

func NewConnectionServer(storage storage.Storage, auth auth.Provider, viewer view.Viewer) connman.ConnectionServiceServer {
	return connman.NewValidatingConnectionServiceServer(&connectionServer{
		storage: storage,
		auth:    auth,
		viewer:  viewer,
	})
}

func (s *connectionServer) Get(ctx context.Context, request *connman.GetConnectionRequest) (*connman.Connection, error) {
	if st := s.auth.Authorize(ctx, PermissionConnectionGet, ResourceConnection(request.Id)); st.Err() != nil {
		return nil, xerrors.Errorf("not enough permissions to get connection: %w", st.Err())
	}

	if request.View == connman.ConnectionView_CONNECTION_VIEW_SENSITIVE {
		if st := s.auth.Authorize(ctx, PermissionConnectionViewSensitive, ResourceConnection(request.Id)); st.Err() != nil {
			return nil, xerrors.Errorf("not enough permissions to view sensitive data for connection: %w", st.Err())
		}
	}

	connection, err := s.storage.Get(ctx, request.Id)
	if err != nil {
		return nil, xerrors.Errorf(errUnableToGetConnection, err)
	}

	return s.viewer.PrepareConnectionView(ctx, connection, request.View)
}

func (s *connectionServer) List(ctx context.Context, request *connman.ListConnectionRequest) (*connman.ListConnectionResponse, error) {
	connections, err := s.storage.List(ctx, request.FolderId)
	if err != nil {
		return nil, xerrors.Errorf("unable to list connections: %w", err)
	}

	response := new(connman.ListConnectionResponse)
	if len(connections) == 0 {
		return response, nil
	}

	resources := make([]cloudauth.Resource, 0, len(connections))
	for _, connection := range connections {
		resources = append(resources, ResourceConnection(connection.ID))
	}

	if st := s.auth.Authorize(ctx, PermissionConnectionGet, resources...); st.Err() != nil {
		return nil, xerrors.Errorf("not enough permissions to get some connections: %w", st.Err())
	}

	if request.View == connman.ConnectionView_CONNECTION_VIEW_SENSITIVE {
		if st := s.auth.Authorize(ctx, PermissionConnectionViewSensitive, resources...); st.Err() != nil {
			return nil, xerrors.Errorf("not enough permissions to view sensitive data for some connections: %w", st.Err())
		}
	}

	for _, connection := range connections {
		connectionProto, err := s.viewer.PrepareConnectionView(ctx, connection, request.View)
		if err != nil {
			return nil, xerrors.Errorf("unable to prepare connection view: %w", err)
		}
		response.Connection = append(response.Connection, connectionProto)
	}
	return response, nil
}

func (s *connectionServer) Create(ctx context.Context, request *connman.CreateConnectionRequest) (*operation.Operation, error) {
	if st := s.auth.Authorize(ctx, PermissionConnectionCreate, cloudauth.ResourceFolder(request.FolderId)); st.Err() != nil {
		return nil, xerrors.Errorf("not enough permissions to create connection in folder '%v': %w", request.FolderId, st.Err())
	}

	userID, err := auth.UserIDFromContext(ctx)
	if err != nil {
		return nil, xerrors.Errorf(auth.ErrUnableToGetUserID, err)
	}

	connection := types.FromCreateConnectionRequest(request, userID)

	var op *types.ConnectionOperation
	err = s.storage.ExecTx(ctx, func(ctx context.Context) error {
		var err error
		op, err = s.storage.Create(ctx, connection)
		return err
	})
	if err != nil {
		return nil, xerrors.Errorf("unable to create connection: %w", err)
	}

	return view.PrepareConnectionOperationView(op, connection)
}

func (s *connectionServer) Update(ctx context.Context, request *connman.UpdateConnectionRequest) (*operation.Operation, error) {
	if st := s.auth.Authorize(ctx, PermissionConnectionUpdate, ResourceConnection(request.Id)); st.Err() != nil {
		return nil, xerrors.Errorf("not enough permissions to update connection: %w", st.Err())
	}

	var connection *types.Connection
	var op *types.ConnectionOperation
	err := s.storage.ExecTx(ctx, func(ctx context.Context) error {
		prev, err := s.storage.Get(ctx, request.Id)
		if err != nil {
			return xerrors.Errorf(errUnableToGetConnection, err)
		}

		connection, err = types.FromUpdateConnectionRequest(prev, request)
		if err != nil {
			return xerrors.Errorf("unable to prepare updated connection: %w", err)
		}

		op, err = s.storage.Update(ctx, connection, prev)
		return err
	})
	if err != nil {
		return nil, xerrors.Errorf("unable to update connection: %w", err)
	}

	return view.PrepareConnectionOperationView(op, connection)
}

func (s *connectionServer) Delete(ctx context.Context, request *connman.DeleteConnectionRequest) (*operation.Operation, error) {
	if st := s.auth.Authorize(ctx, PermissionConnectionDelete, ResourceConnection(request.Id)); st.Err() != nil {
		return nil, xerrors.Errorf("not enough permissions to delete connection: %w", st.Err())
	}

	var op *types.ConnectionOperation
	err := s.storage.ExecTx(ctx, func(ctx context.Context) error {
		var err error
		op, err = s.storage.Delete(ctx, request.Id)
		return err
	})
	if err != nil {
		return nil, xerrors.Errorf("unable to delete connection: %w", err)
	}

	return view.PrepareConnectionOperationView(op, nil)
}
