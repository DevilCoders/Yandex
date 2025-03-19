FROM alpine:3.6

RUN apk add --no-cache \
  nginx-mod-http-lua \
  lua5.1-cjson \
  vim

# Replace nginx configurations
COPY nginx.conf /etc/nginx/nginx.conf

# Copy stubs of external libraries used by S3Lib
COPY yarl-go.lua /etc/nginx/include/yarl/yarl-go.lua

# Create folder for PID file
RUN mkdir -p /run/nginx

EXPOSE 80/tcp
EXPOSE 443/tcp

CMD ["nginx"]
