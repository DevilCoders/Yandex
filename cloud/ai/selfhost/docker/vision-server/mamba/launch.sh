# Start the first process
./cbirdaemon2/cbirdaemon2 --port 11122 --data-dir ./vision/launch_cbirdaemon2/data --threads $(nproc) --config ./vision/launch_cbirdaemon2/prod_configs/cbir_prod_config.conf &
#./cbirdaemon2/cbirdaemon2 --port 11122 --data-dir ./vision/launch_cbirdaemon2/data --threads $(nproc) --config ./vision/launch_cbirdaemon2/prod_configs/cbirdaemon_3heads.conf &
status=$?
if [ $status -ne 0 ]; then
  echo "Failed to start cbirdaemon2 with status: $status"
  exit $status
fi

# Start the second process
./vision/python/server/server --config ./vision/python/server/custom_config.json &
status=$?
if [ $status -ne 0 ]; then
  echo "Failed to start classifier service server with status: $status"
  exit $status
fi

#warmup
sleep 30
echo "doing warmup!"
./vision/python/client/client -p ./vision/launch_cbirdaemon2/data/warmup_image.jpg -H 'request_id:warmup'

# Naive check runs checks once a minute to see if either of the processes exited.
# This illustrates part of the heavy lifting you need to do if you want to run
# more than one service in a container. The container exits with an error
# if it detects that either of the processes has exited.
# Otherwise it loops forever, waking up every 60 seconds

while sleep 60; do
  ps aux |grep cbirdaemon2 |grep -q -v grep
  PROCESS_1_STATUS=$?
  ps aux |grep server |grep -q -v grep
  PROCESS_2_STATUS=$?
  # If the greps above find anything, they exit with 0 status
  # If they are not both 0, then something is wrong
  if [ $PROCESS_1_STATUS -ne 0 -o $PROCESS_2_STATUS -ne 0 ]; then
    echo "One of the processes has already exited."
    exit 1
  fi
done
