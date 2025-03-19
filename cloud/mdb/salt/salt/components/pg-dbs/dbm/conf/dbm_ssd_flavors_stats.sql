WITH dom0_hosts AS (
    SELECT
        -- common fields

        i.generation,
        geo,

        -- 4 vCPU, 16 GB RAM, 50GB SSD
        free_cores::decimal / 4 as free_cores_rank_4,
        free_ssd::decimal / 53687091200 as free_ssd_rank_4,
        free_memory::decimal / 17179869184 as free_memory_rank_4,

        GREATEST(free_cores::decimal-r.cpu_cores,0) / 4 as reserved_free_cores_rank_4,
        GREATEST(free_ssd::decimal - r.ssd_space,0) / 53687091200 as reserved_free_ssd_rank_4,
        GREATEST(free_memory::decimal - r.memory,0) / 17179869184 as reserved_free_memory_rank_4,

        -- 8 vCPU, 32 GB RAM, 100GB SSD
        free_cores::decimal / 8 as free_cores_rank_8,
        free_ssd::decimal / 107374182400 as free_ssd_rank_8,
        free_memory::decimal / 34359738368 as free_memory_rank_8,

        GREATEST(free_cores::decimal-r.cpu_cores,0) / 8 as reserved_free_cores_rank_8,
        GREATEST(free_ssd::decimal - r.ssd_space,0) / 107374182400 as reserved_free_ssd_rank_8,
        GREATEST(free_memory::decimal - r.memory,0) / 34359738368 as reserved_free_memory_rank_8,

        -- 16 vCPU, 64 GB RAM, 200GB SSD
        free_cores::decimal / 16 as free_cores_rank_16,
        free_ssd::decimal / 214748364800 as free_ssd_rank_16,
        free_memory::decimal / 68719476736 as free_memory_rank_16,

        GREATEST(free_cores::decimal-r.cpu_cores,0) / 16 as reserved_free_cores_rank_16,
        GREATEST(free_ssd::decimal - r.ssd_space,0) / 214748364800 as reserved_free_ssd_rank_16,
        GREATEST(free_memory::decimal - r.memory,0) / 68719476736 as reserved_free_memory_rank_16,

        -- 32 vCPU, 128 GB RAM, 1024GB SSD
        free_cores::decimal / 32 as free_cores_rank_32,
        free_ssd::decimal / 1099511627776 as free_ssd_rank_32,
        free_memory::decimal / 137438953472 as free_memory_rank_32,

        GREATEST(free_cores::decimal-r.cpu_cores,0) / 32 as reserved_free_cores_rank_32,
        GREATEST(free_ssd::decimal - r.ssd_space,0) / 1099511627776 as reserved_free_ssd_rank_32,
        GREATEST(free_memory::decimal - r.memory,0) / 137438953472 as reserved_free_memory_rank_32,

        -- 48 vCPU, 192 GB RAM, 2048GB SSD
        free_cores::decimal / 48 as free_cores_rank_48,
        free_ssd::decimal / 2199023255552 as free_ssd_rank_48,
        free_memory::decimal / 206158430208 as free_memory_rank_48,

        GREATEST(free_cores::decimal-r.cpu_cores,0) / 48 as reserved_free_cores_rank_48,
        GREATEST(free_ssd::decimal - r.ssd_space,0) / 2199023255552 as reserved_free_ssd_rank_48,
        GREATEST(free_memory::decimal - r.memory,0) / 206158430208 as reserved_free_memory_rank_48,

        -- 64 vCPU, 256 GB RAM, 2000GB SSD -- special for CPU intensive clusters
        free_cores::decimal / 64 as free_cores_rank_64,
        free_ssd::decimal / 2199023255552 as free_ssd_rank_64,
        free_memory::decimal / 274877906944 as free_memory_rank_64,

        GREATEST(free_cores::decimal-r.cpu_cores,0) / 64 as reserved_free_cores_rank_64,
        GREATEST(free_ssd::decimal - r.ssd_space,0) / 2199023255552 as reserved_free_ssd_rank_64,
        GREATEST(free_memory::decimal - r.memory,0) / 274877906944 as reserved_free_memory_rank_64,

        -- 384 GB RAM
        free_memory::decimal / (384 * power(2,30)) as free_memory_rank_96,
        GREATEST(free_memory::decimal - r.memory,0) / (384 * power(2,30))as reserved_free_memory_rank_96,

        -- 512 GB RAM
        free_memory::decimal / (512 * power(2,30)) as free_memory_rank_128,
        GREATEST(free_memory::decimal - r.memory,0) / (512 * power(2,30))as reserved_free_memory_rank_128

    FROM mdb.dom0_info AS i
    FULL JOIN mdb.reserved_resources AS r ON i.generation=r.generation
    WHERE
      (i.project = 'pgaas') and heartbeat > now()::date - 1 and i.allow_new_hosts = True AND i.free_ssd > 0
)
SELECT geo,
       generation as gen,
       -- 4 vCPU, 16 GB RAM, 50GB SSD
       sum(floor(least(free_memory_rank_4, free_cores_rank_4, free_ssd_rank_4))) as s_4vcpu_16ram_50ssd,
       sum(floor(least(reserved_free_memory_rank_4, reserved_free_cores_rank_4, reserved_free_ssd_rank_4))) as reserved_s_4vcpu_16ram_50ssd,
       -- 4 vCPU, 32 GB RAM, 50GB SSD
       sum(floor(least(free_memory_rank_8, free_cores_rank_4, free_ssd_rank_4))) as m_4vcpu_32ram_50ssd,
       sum(floor(least(reserved_free_memory_rank_8, reserved_free_cores_rank_4, reserved_free_ssd_rank_4))) as reserved_m_4vcpu_32ram_50ssd,
       -- 8 vCPU, 32 GB RAM, 100GB SSD
       sum(floor(least(free_memory_rank_8, free_cores_rank_8, free_ssd_rank_8))) as s_8vcpu_32ram_100ssd,
       sum(floor(least(reserved_free_memory_rank_8, reserved_free_cores_rank_8, reserved_free_ssd_rank_8))) as reserved_s_8vcpu_32ram_100ssd,
       -- 8 vCPU, 64 GB RAM, 100GB SSD
       sum(floor(least(free_memory_rank_16, free_cores_rank_8, free_ssd_rank_8))) as m_8vcpu_64ram_100ssd,
       sum(floor(least(reserved_free_memory_rank_16, reserved_free_cores_rank_8, reserved_free_ssd_rank_8))) as reserved_m_8vcpu_64ram_100ssd,
       -- 16 vCPU, 64 GB RAM, 200GB SSD
       sum(floor(least(free_memory_rank_16, free_cores_rank_16, free_ssd_rank_16))) as s_16vcpu_64ram_200ssd,
       sum(floor(least(reserved_free_memory_rank_16, reserved_free_cores_rank_16, reserved_free_ssd_rank_16))) as reserved_s_16vcpu_64ram_200ssd,
       -- 16 vCPU, 128 GB RAM, 200GB SSD
       sum(floor(least(free_memory_rank_32, free_cores_rank_16, free_ssd_rank_16))) as m_16vcpu_128ram_200ssd,
       sum(floor(least(reserved_free_memory_rank_32, reserved_free_cores_rank_16, reserved_free_ssd_rank_16))) as reserved_m_16vcpu_128ram_200ssd,
       -- 32 vCPU, 128 GB RAM, 1024GB SSD
       sum(floor(least(free_memory_rank_32, free_cores_rank_32, free_ssd_rank_32))) as s_32vcpu_128ram_1024ssd,
       sum(floor(least(reserved_free_memory_rank_32, reserved_free_cores_rank_32, reserved_free_ssd_rank_32))) as reserved_s_32vcpu_128ram_1024ssd,
       -- 32 vCPU, 256 GB RAM, 1024GB SSD
       sum(floor(least(free_memory_rank_64, free_cores_rank_32, free_ssd_rank_32))) as m_32vcpu_256ram_1024ssd,
       sum(floor(least(reserved_free_memory_rank_64, reserved_free_cores_rank_32, reserved_free_ssd_rank_32))) as reserved_m_32vcpu_256ram_1024ssd,
       -- 48 vCPU, 192 GB RAM, 2048GB SSD
       sum(floor(least(free_memory_rank_48, free_cores_rank_48, free_ssd_rank_48))) as s_48vcpu_192ram_2048ssd,
       sum(floor(least(reserved_free_memory_rank_48, reserved_free_cores_rank_48, reserved_free_ssd_rank_48))) as reserved_s_48vcpu_192ram_2048ssd,
       -- 48 vCPU, 384 GB RAM, 2048GB SSD
       sum(floor(least(free_memory_rank_96, free_cores_rank_48, free_ssd_rank_48))) as m_48vcpu_384ram_2048ssd,
       sum(floor(least(reserved_free_memory_rank_96, reserved_free_cores_rank_48, reserved_free_ssd_rank_48))) as reserved_m_48vcpu_384ram_2048ssd,
       -- 64 vCPU, 256 GB RAM, 2048GB SSD -- special for CPU intensive clusters
       sum(floor(least(free_memory_rank_64, free_cores_rank_64, free_ssd_rank_64))) as s_64vcpu_256ram_2048ssd,
       sum(floor(least(reserved_free_memory_rank_64, reserved_free_cores_rank_64, reserved_free_ssd_rank_64))) as reserved_s_64cpu_64ram_2048ssd,
       -- 64 vCPU, 512 GB RAM, 2048GB SSD -- special for CPU intensive clusters
       sum(floor(least(free_memory_rank_128, free_cores_rank_64, free_ssd_rank_64))) as m_64vcpu_256ram_2048ssd,
       sum(floor(least(reserved_free_memory_rank_128, reserved_free_cores_rank_64, reserved_free_ssd_rank_64))) as reserved_m_64vcpu_128ram_2048ssd,
       -- 16 vCPU, 32 GB RAM, 2048GB SSD -- special for ya mail
       sum(floor(least(free_memory_rank_16, free_cores_rank_16, free_ssd_rank_64))) as s_16vcpu_64ram_2048ssd,
       sum(floor(least(reserved_free_memory_rank_16, reserved_free_cores_rank_16, reserved_free_ssd_rank_64))) as reserved_s_16vcpu_64ram_2048ssd
FROM dom0_hosts
GROUP BY geo, gen;
