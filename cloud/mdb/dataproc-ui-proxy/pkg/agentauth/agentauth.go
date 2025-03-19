package agentauth

import (
	"net/http"
	"strings"

	intapi "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/internal-api"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/invertingproxy/common"
	"a.yandex-team.ru/cloud/mdb/dataproc-ui-proxy/pkg/util"
	"a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const permission = "dataproc.agent.proxyUi"

type AgentAuth struct {
	internalAPI   intapi.InternalAPI
	accessService accessservice.AccessService
	renderError   util.RenderError
	logger        log.Logger
}

func New(internalAPI intapi.InternalAPI, accessService accessservice.AccessService, renderError util.RenderError, logger log.Logger) *AgentAuth {
	if renderError == nil {
		renderError = util.NewErrorRenderer(false, logger)
	}
	return &AgentAuth{
		internalAPI:   internalAPI,
		accessService: accessService,
		renderError:   renderError,
		logger:        logger,
	}
}

func (a *AgentAuth) auth(r *http.Request) error {
	if a.accessService == nil {
		return nil
	}

	ctx := r.Context()
	clusterID := r.Header.Get(common.HeaderAgentID)
	topology, err := a.internalAPI.GetClusterTopology(ctx, clusterID)
	if err != nil {
		return xerrors.Errorf("failed to fetch cluster topology from intapi: %w", err)
	}

	const prefix = "Bearer "
	header := r.Header.Get("Authorization")
	iamToken := strings.TrimPrefix(header, prefix)
	resourceFolder := accessservice.ResourceFolder(topology.FolderID)
	_, err = a.accessService.Auth(ctx, iamToken, permission, resourceFolder)
	return err
}

func (a *AgentAuth) Middleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		err := a.auth(r)
		if err != nil {
			err = xerrors.Errorf("agent request auth check failed: %w", err)
			a.renderError(w, err)
			return
		}

		next.ServeHTTP(w, r)
	})
}
