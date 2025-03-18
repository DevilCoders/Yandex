#!/bin/sh

revision="$1"
shift
if [ -z "$revision" ]; then
    echo "Please specify revision: $0 <revision number>"
    exit 2
fi

echo "Updating proxima v9 metrics to revision $revision"

if [ -x "./metrics" ]; then
    echo "Please build <arcadia>/search/metrics/monitoring tagret and make symlink 'metrics' binary to current folder"
    exit 2
fi

modules="proxima_v9_metrics proxima_v9_component_metrics proxima_v9_custom_metrics proxima_v9_judged_metrics"

for module in $modules; do
    echo "Updating metrics with module $module"
    ./metrics tool find metric --module "$module" | ./metrics tool update metric --revision "$revision"
done
