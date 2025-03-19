# Lazy TRIM

This small utility we are using for routine everyday trimming ssd disks.
Everyday trimming give some expected and predicted latency of I/O operations and increasing service life of disks.

Throttling feature will decrease side effects of increased workload.

You doesn't need to mount your disks with disk option `discard`, because it causes unexpected latency spikes.
Also, you doesn't need to run `fstrim` on you disks time to time.

Just run this utility everyday, before you will go to your bed.
Enjoy!

## TODO
* Smart throttling. If trim_time is low, than increase block_size / bandwith. If trim_time is high, than decrease them.
* Auto-scaling throttling based on metircs from psutil.disk_io_counters and iowait%
* Write last summary to file /var/run/lazy-trim (start_ts, end_ts, trimmed_blocks
