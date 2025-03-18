package adminhandler

import (
	"fmt"
	"net/http"

	"a.yandex-team.ru/cdn/cloud_api/pkg/service/adminservice"
	"a.yandex-team.ru/library/go/core/log"
)

type Handler struct {
	Logger       log.Logger
	AdminService adminservice.Service
}

func (h *Handler) RunDatabaseGC(w http.ResponseWriter, r *http.Request) {
	err := h.AdminService.RunDatabaseGC()
	if err != nil {
		h.Logger.Error("database gc failed", log.Error(err))
		http.Error(w, fmt.Sprintf("database gc failed: %v", err), http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusOK)
	_, _ = w.Write([]byte("database gc finished\n"))
}
