package http

import (
	"context"
	"net/http"
	"time"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"

	"a.yandex-team.ru/cloud/marketplace/pkg/auth/access-backend"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/actions"
)

func (s *Service) initRouter() http.Handler {
	router := chi.NewRouter()

	router.Route("/marketplace/v2/license", func(r chi.Router) {
		r.Use(
			requestIDCtx,
			s.recoverMiddleware,
			authCtx,
		)

		r.MethodNotAllowed(s.makeMethodNotAllowedHandler())

		r.Post("/check", s.registerHandler("LicenseCheck", s.licenseCheckHandler))
	})

	return router
}

type writerWithCheck struct {
	http.ResponseWriter

	status  int
	touched bool
}

func (w *writerWithCheck) Write(b []byte) (int, error) {
	w.touched = true
	return w.ResponseWriter.Write(b)
}

func (w *writerWithCheck) WriteHeader(status int) {
	w.touched = true
	w.status = status

	w.ResponseWriter.WriteHeader(status)
}

func (s *Service) registerHandler(handlerName string, handler wrappedHandler) http.HandlerFunc {
	s.Env.Metrics().RegisterHTTPHandler(handlerName)
	return s.withPrologue(handlerName, handler)
}

type wrappedHandler func(w http.ResponseWriter, r *http.Request) error

func (s *Service) withPrologue(handlerName string, handler wrappedHandler) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		writer := &writerWithCheck{
			ResponseWriter: w,
		}

		traceSpan, traceCtx := tracing.Start(r.Context(), handlerName)
		defer traceSpan.Finish()

		var apiErr *apiError

		start := time.Now()
		err := handler(writer, r.WithContext(traceCtx))
		span := time.Since(start)

		defer func() {
			httpStatus := httpStatus(traceCtx, apiErr, writer)

			s.Metrics().CommitHTTPRequest(traceCtx, handlerName, httpStatus, span)
			ctxtools.Logger(r.Context()).Info("request completed",
				accessLogFieldsWithTags(traceCtx,
					log.Duration("request_time", span),
					log.String("user_agent", r.UserAgent()),
					log.String("remote_ip", r.RemoteAddr),
					log.String("path", r.URL.Path),
					log.Int("http_status", httpStatus),
				)...,
			)
		}()

		if err == nil {
			logging.Logger().Debug("request processed")
			return
		}

		scopedLogger := logging.LoggerWith(
			log.Error(err),
		)

		scopedLogger.Error("failed to process http request")

		if writer.touched {
			scopedLogger.Error("handler completed with error, but response was already written")
			return
		}

		var (
			licenseCheckErr         actions.ErrLicenseCheck
			licenseCheckExternalErr actions.ErrLicenseCheckExternal
			notfoundCloudIDErr      actions.ErrNotfoundCloudID

			anyAPIErr = &apiError{}
		)

		switch {
		case xerrors.As(err, &anyAPIErr):
			apiErr = anyAPIErr
		case xerrors.As(err, &licenseCheckExternalErr):
			apiErr = newErrLicenseCheckExternal(
				licenseCheckExternalErr.CloudID,
				licenseCheckExternalErr.ProductsIDs,
				licenseCheckExternalErr.ExternalMessages,
			)
		case xerrors.As(err, &licenseCheckErr):
			apiErr = errLicenseCheck
		case xerrors.Is(err, actions.ErrNoCloudID):
			apiErr = errNoCloudID
		case xerrors.As(err, &notfoundCloudIDErr):
			apiErr = newErrCloudIDNotFound(notfoundCloudIDErr.CloudID)
		case xerrors.Is(err, actions.ErrLicensedInstancePoolValue):
			apiErr = errLicensedInstancePoolValue
		case xerrors.Is(err, actions.ErrBackendTimeout):
			apiErr = errBackendTimeout
		case xerrors.Is(err, actions.ErrEmptyProductIDs):
			apiErr = errEmptyProductsIDs
		case xerrors.Is(err, access.ErrPermissionDenied):
			apiErr = errAPIUnauthorized
		case xerrors.Is(err, access.ErrMalformedToken):
			apiErr = errAPIAuthFailure
		case xerrors.Is(err, access.ErrUnauthenticated):
			apiErr = errAPIAuthFailure
		case xerrors.Is(err, access.ErrMissingAuthToken):
			apiErr = errAPIAuthFailure
		case xerrors.Is(err, context.Canceled), xerrors.Is(err, context.DeadlineExceeded):
			scopedLogger.Info("context cancelled or deadline exceeded", log.Error(err))
			return
		default:
			apiErr = errInternal
			scopedLogger.Error("sending internal service error to client")
		}

		s.sendAPIError(r.Context(), w, apiErr)
	}
}

func httpStatus(ctx context.Context, apiErr *apiError, w *writerWithCheck) int {
	httpStatus := http.StatusInternalServerError

	switch {
	case xerrors.Is(ctx.Err(), context.Canceled), xerrors.Is(ctx.Err(), context.DeadlineExceeded):
		// Special case: client cancelled the request.
		ctxtools.Logger(ctx).Debug("request context has been cancelled", log.Error(ctx.Err()))
		httpStatus = 499
	case apiErr != nil:
		httpStatus = apiErr.status
	case w != nil && w.touched:
		httpStatus = w.status
	}

	return httpStatus
}

func accessLogFieldsWithTags(ctx context.Context, fields ...log.Field) []log.Field {
	logFields := make([]log.Field, 0, len(fields))
	logFields = append(logFields, fields...)

	for tagName, tagValues := range ctxtools.GetTags(ctx) {
		switch len(tagValues) {
		case 0:
			continue
		case 1:
			logFields = append(logFields, log.String(tagName, tagValues[0]))
		default:
			logFields = append(logFields, log.Strings(tagName, tagValues))
		}
	}

	return logFields
}
