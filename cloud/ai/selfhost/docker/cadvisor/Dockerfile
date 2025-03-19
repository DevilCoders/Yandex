ARG version
FROM google/cadvisor:$version

EXPOSE 17008

HEALTHCHECK --interval=30s --timeout=3s \
  CMD wget --quiet --tries=1 --spider http://localhost:17008/healthz || exit 1
