// DO NOT EDIT IN-PLACE, please update script in repo:
// tools/mstand/serp/resource_calculation_script.groovy

// formula: base + factor * serpsets-size

// MSTAND-1725: add some place for metric logs
MB = 1024 * 1024
disk_base = 64
metric_disk_factor = 0.015

threads_limit = 8

// keep in sync with OfflineDefaultValues.LOCAL_MODE_MAX_METRICS
local_mode_max_metrics_number = 8
local_mode_max_serpsets_disk_size_mb = 16

ram_base = 64
// MSTAND-1604: lower ram factor
metric_ram_factor = 2.0
// for serpset parsing/conversion, etc
cpu_factor = 0.9
calc_mode = "yt"
yt_output_ttl = "5d"

in0.each { input ->
  metrics_number = input["metrics-number"]
}

in1.each { input ->
  serpsets_disk_size = input["serpsets-disk-size"]
  serpsets_number = input["serpsets-number"]
}

threads_number = Math.min(serpsets_number, threads_limit)

serpsets_disk_size_mb = serpsets_disk_size / MB

if (serpsets_disk_size_mb < 16) {
    serp_ram_factor = 600
    yt_serp_ram_factor = 1000
} else if (serpsets_disk_size_mb < 32) {
    serp_ram_factor = 650
    yt_serp_ram_factor = 1300
} else if (serpsets_disk_size_mb < 48) {
    serp_ram_factor = 700
    yt_serp_ram_factor = 1600
} else if (serpsets_disk_size_mb < 64) {
    serp_ram_factor = 750
    yt_serp_ram_factor = 1900
} else {
    serp_ram_factor = 800
    yt_serp_ram_factor = 2100
}

if (metrics_number < local_mode_max_metrics_number) {
    // very small batch => local mode
    calc_mode = "local"
} else if (metrics_number < local_mode_max_metrics_number * 4 &&
    serpsets_disk_size_mb < local_mode_max_serpsets_disk_size_mb) {
    // medium-sized batch and small serpsets => local mode
    calc_mode = "local"
}

if (calc_mode == "yt") {
    metric_disk_factor = 0.0
    metric_ram_factor = 1.0
    // MSTAND-1816: increase serp ram factor for YT mode:
    // serpset upload takes more ram
    serp_ram_factor += yt_serp_ram_factor
    cpu_factor = 0.25
    // for light serps, don't require much CPU for serpset conversion
    if (serpsets_disk_size_mb < 50) {
        cpu_factor = 0.15
    }
}

// MSTAND-1720: serpset conversion to jsonlines eats 2 x (1 serpset size)
// so we need 2x place for serpsets
// See latest examples in MSTAND-1807 (extra large serps)
// MSTAND-1816: calculate ram separately - for tmpfs=false cases (see serp_ram_factor)
serp_disk_factor = 35 + threads_number

force_tmpfs_disk = false

serp_storage = serp_disk_factor * serpsets_disk_size_mb
metric_storage = metric_disk_factor * serpsets_disk_size_mb * metrics_number
// after MSTAND-1711, metric results disk usage is insignificant.
max_disk_mb = disk_base + serp_storage + metric_storage

metrics_ram = metric_ram_factor * threads_number * metrics_number
// for serpset conversion/parsing, etc.
serps_ram = serp_ram_factor * threads_number
max_ram_mb = ram_base + metrics_ram + serps_ram

if (max_ram_mb + max_disk_mb < 64000) {
  force_tmpfs_disk = true
}

cpu_guarantee = 100 * threads_number * cpu_factor

output_params = [
  "force-tmpfs-disk": force_tmpfs_disk,
  "max-disk": max_disk_mb,
  "max-ram": max_ram_mb,
  "threads": threads_number,
  "cpu-guarantee": cpu_guarantee,
  "serp-storage-mb": serp_storage,
  "metric-storage-mb": metric_storage,
  "metrics-number": metrics_number,
  "serpsets-number": serpsets_number,
  "serp-disk-factor": serp_disk_factor,
  "serp-ram-factor": serp_ram_factor,
  "serpsets-disk-size": serpsets_disk_size_mb,
  "yt-output-ttl": yt_output_ttl,
  "calc-mode": calc_mode,
]

out.write(output_params)
