# https://console.cloud.google.com/gcr/images/distroless/GLOBAL/base-debian10?gcrImageListsize=30
FROM gcr.io/distroless/base-debian10@sha256:1751abf842fee4b5c8662efee388b09a7f569158da8e6b7a0100be7e728eb3cb
# For debuggin use docker run -it --entrypoint sh with debug image:
# FROM gcr.io/distroless/base-debian10:debug

COPY unified_agent /usr/local/bin/unified_agent

ENTRYPOINT ["/usr/local/bin/unified_agent"]
