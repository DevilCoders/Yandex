cat << EOF
--module-root crowdkit \
--src-root $(dirname $0)/../../../crowd-kit/crowdkit \
--output-dir $(dirname $0)/../../../crowd-kit/docs/reference/ \
--github-source-url https://github.com/Toloka/crowd-kit/blob/v1.0.0/crowdkit
EOF
