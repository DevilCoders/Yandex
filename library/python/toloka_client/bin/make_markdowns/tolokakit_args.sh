cat << EOF
--module-root toloka \
--src-root $(dirname $0)/../../../toloka-kit/src/ \
--output-dir $(dirname $0)/../../../toloka-kit/docs/reference/ \
--github-source-url https://github.com/Toloka/toloka-kit/blob/v0.1.26/src
EOF
