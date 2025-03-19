CREATE TABLE mdb.reserved_resources (
    generation integer NOT NULL,
    cpu_cores  integer NOT NULL,
    memory     bigint  NOT NULL,
    io         bigint  NOT NULL,
    net        bigint  NOT NULL,
    ssd_space  bigint  NOT NULL,

    CONSTRAINT pk_reserved_resources PRIMARY KEY (generation),

    CONSTRAINT check_cpu_cores CHECK (
        cpu_cores >= 0
    ),

    CONSTRAINT check_memory CHECK (
        memory >= 0
    ),

    CONSTRAINT check_io CHECK (
        io >= 0
    ),

    CONSTRAINT check_net CHECK (
        net >= 0
    ),

    CONSTRAINT check_ssd_space CHECK (
        ssd_space >= 0
    )
);

INSERT INTO mdb.reserved_resources (
    generation,
    cpu_cores,
    memory,
    io,
    net,
    ssd_space
) VALUES (
    1,
    16,
    68719476736,
    335544320,
    33554432,
    1717986918400
), (
    2,
    40,
    171798691840,
    671088640,
    671088640,
    4294967296000
), (
    3,
    64,
    274877906944,
    1073741824,
    1073741824,
    6871947673600
);
