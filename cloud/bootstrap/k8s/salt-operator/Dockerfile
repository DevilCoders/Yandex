# Use distroless as minimal base image to package the manager binary
# Refer to https://github.com/GoogleContainerTools/distroless for more details
# For debug use gcr.io/distroless/base:debug-nonroot and sh as ENTRYPOINT
FROM gcr.io/distroless/base:nonroot
WORKDIR /
COPY --chown=nonroot:nonroot salt-operator-manager /
USER 65532:65532

ENTRYPOINT ["/salt-operator-manager"]
