WITH dom0_hosts AS (
    SELECT
        -- common fields
        i.generation,
        geo,
        (free_raw_disks_space::decimal/free_raw_disks) / 13743895347200 as free_sata_rank,

        free_cores::decimal / 8 as free_cores_rank_8,
        free_memory::decimal / (32 * power(2,30)) as free_memory_rank_32,
        GREATEST(free_cores::decimal-r.cpu_cores,0) / 8 as reserved_free_cores_rank_8,
        GREATEST(free_memory::decimal - r.memory,0) / (32 * power(2,30)) as reserved_free_memory_rank_32,

        free_cores::decimal / 12 as free_cores_rank_12,
        free_memory::decimal / (48 * power(2, 30)) as free_memory_rank_48,
        GREATEST(free_cores::decimal-r.cpu_cores,0) / 12 as reserved_free_cores_rank_12,
        GREATEST(free_memory::decimal - r.memory,0) / (48 * power(2,30)) as reserved_free_memory_rank_48,

        free_cores::decimal / 16 as free_cores_rank_16,
        free_memory::decimal / (64 * power(2, 30)) as free_memory_rank_64,
        GREATEST(free_cores::decimal-r.cpu_cores,0) / 16 as reserved_free_cores_rank_16,
        GREATEST(free_memory::decimal - r.memory,0) / (64 * power(2,30)) as reserved_free_memory_rank_64,

        free_cores::decimal / 24 as free_cores_rank_24,
        free_memory::decimal / (96 * power(2, 30)) as free_memory_rank_96,
        GREATEST(free_cores::decimal-r.cpu_cores,0) / 24 as reserved_free_cores_rank_24,
        GREATEST(free_memory::decimal - r.memory,0) / (96 * power(2,30)) as reserved_free_memory_rank_96,

        free_cores::decimal / 32 as free_cores_rank_32,
        free_memory::decimal / (128 * power(2, 30)) as free_memory_rank_128,
        GREATEST(free_cores::decimal-r.cpu_cores,0) / 32 as reserved_free_cores_rank_32,
        GREATEST(free_memory::decimal - r.memory,0) / (128 * power(2,30)) as reserved_free_memory_rank_128,

        free_cores::decimal / 40 as free_cores_rank_40,
        free_memory::decimal / (160 * power(2, 30)) as free_memory_rank_160,
        GREATEST(free_cores::decimal-r.cpu_cores,0) / 40 as reserved_free_cores_rank_40,
        GREATEST(free_memory::decimal - r.memory,0) / (160 * power(2,30)) as reserved_free_memory_rank_160,

        free_cores::decimal / 48 as free_cores_rank_48,
        free_memory::decimal / (192 * power(2, 30)) as free_memory_rank_192,
        GREATEST(free_cores::decimal-r.cpu_cores,0) / 48 as reserved_free_cores_rank_48,
        GREATEST(free_memory::decimal - r.memory,0) / (192 * power(2,30)) as reserved_free_memory_rank_192,

        free_cores::decimal / 64 as free_cores_rank_64,
        free_memory::decimal / (256 * power(2, 30)) as free_memory_rank_256,
        GREATEST(free_cores::decimal-r.cpu_cores,0) / 64 as reserved_free_cores_rank_64,
        GREATEST(free_memory::decimal - r.memory,0) / (256 * power(2,30)) as reserved_free_memory_rank_256
    FROM mdb.dom0_info i
    FULL JOIN mdb.reserved_resources AS r ON i.generation=r.generation
    WHERE
      (i.project = 'pgaas') and  (free_raw_disks > 0) and heartbeat > now()::date - 1 and i.generation >= 3 and i.allow_new_hosts = True
)
SELECT geo,
       generation as gen,
       sum(floor(least(free_memory_rank_32, free_cores_rank_8, free_sata_rank))) as s_8vcpu_32ram_12800hdd,
       sum(floor(least(reserved_free_memory_rank_32, reserved_free_cores_rank_8))) as reserved_s_8vcpu_32ram_12800hdd,
       sum(floor(least(free_memory_rank_48, free_cores_rank_12, free_sata_rank))) as s_12vcpu_48ram_12800hdd,
       sum(floor(least(reserved_free_memory_rank_48, reserved_free_cores_rank_12))) as reserved_s_12vcpu_48ram_12800hdd,
       sum(floor(least(free_memory_rank_64, free_cores_rank_16, free_sata_rank))) as s_16vcpu_64ram_12800hdd,
       sum(floor(least(reserved_free_memory_rank_64, reserved_free_cores_rank_16))) as reserved_s_16vcpu_64ram_12800hdd,
       sum(floor(least(free_memory_rank_96, free_cores_rank_24, free_sata_rank))) as s_24vcpu_96ram_12800hdd,
       sum(floor(least(reserved_free_memory_rank_96, reserved_free_cores_rank_24))) as reserved_s_24vcpu_96ram_12800hdd,
       sum(floor(least(free_memory_rank_128, free_cores_rank_32, free_sata_rank))) as s_32vcpu_128ram_12800hdd,
       sum(floor(least(reserved_free_memory_rank_128, reserved_free_cores_rank_32))) as reserved_s_32vcpu_128ram_12800hdd,
       sum(floor(least(free_memory_rank_160, free_cores_rank_40, free_sata_rank))) as s_40vcpu_160ram_12800hdd,
       sum(floor(least(reserved_free_memory_rank_160, reserved_free_cores_rank_40))) as reserved_s_40vcpu_160ram_12800hdd,
       sum(floor(least(free_memory_rank_192, free_cores_rank_48, free_sata_rank))) as s_48vcpu_192ram_12800hdd,
       sum(floor(least(reserved_free_memory_rank_192, reserved_free_cores_rank_48))) as reserved_s_48vcpu_192ram_12800hdd,
       sum(floor(least(free_memory_rank_256, free_cores_rank_64, free_sata_rank))) as s_64vcpu_256ram_12800hdd,
       sum(floor(least(reserved_free_memory_rank_256, reserved_free_cores_rank_64))) as reserved_s_64vcpu_256ram_12800hdd

FROM dom0_hosts
GROUP BY geo, gen;
