CREATE TABLE s3.object_locks
(
    bid              uuid    NOT NULL,
    name             text COLLATE "C" NOT NULL,
    CONSTRAINT pk_object_locks PRIMARY KEY (bid, name)
);

CREATE TABLE s3.objects_noncurrent
(
    bid              uuid    NOT NULL,
    name             text COLLATE "C" NOT NULL,
    created          timestamp with time zone NOT NULL,
    noncurrent       timestamp with time zone NOT NULL DEFAULT current_timestamp,
    null_version     boolean,
    delete_marker    boolean,
    data_size        bigint,
    data_md5         uuid,
    mds_couple_id    integer,
    mds_key_version  integer,
    mds_key_uuid     uuid,
    parts_count      integer,
    parts            s3.object_part[],
    storage_class    integer,
    creator_id       text COLLATE "C",
    metadata         jsonb,
    acl              jsonb,

    CONSTRAINT pk_objects_noncurrent PRIMARY KEY (bid, name, created),

    CONSTRAINT check_object_name CHECK (
        int4range(1, 1025) @> octet_length(convert_to(name, 'UTF8'))
    ),

    CONSTRAINT check_data CHECK (
        (delete_marker IS true AND mds_couple_id IS NULL
            AND mds_key_version IS NULL AND mds_key_uuid IS NULL
            AND data_size IS NULL AND data_md5 IS NULL
            AND parts_count IS NULL AND parts IS NULL)
        OR
        (delete_marker IS false AND mds_couple_id IS NOT NULL
            AND mds_key_version IS NOT NULL AND mds_key_uuid IS NOT NULL
            AND data_size IS NOT NULL AND data_md5 IS NOT NULL
            AND parts_count IS NULL AND parts IS NULL)
        OR
        -- Empty object could be not uploaded to MDS
        (delete_marker IS false AND mds_couple_id IS NULL
            AND mds_key_version IS NULL AND mds_key_uuid IS NULL
            AND data_size = 0 and data_md5 IS NOT NULL
            AND parts_count IS NULL AND parts IS NULL)
        OR
        -- Multipart object could have no "metadata" record in MDS
        (delete_marker IS false AND mds_couple_id IS NULL
            AND mds_key_version IS NULL AND mds_key_uuid IS NULL
            AND data_size IS NOT NULL AND data_md5 IS NOT NULL
            AND parts_count IS NOT NULL AND parts IS NOT NULL)
        OR
        -- Multipart object with "metadata" record in MDS
        (delete_marker IS false AND mds_couple_id IS NOT NULL
            AND mds_key_version IS NOT NULL AND mds_key_uuid IS NOT NULL
            AND data_size IS NOT NULL AND data_md5 IS NOT NULL
            AND parts_count IS NOT NULL AND parts IS NOT NULL)
        OR
        -- object migrated from YDB
        (metadata IS NOT NULL AND (metadata->>'migrated')::bool IS true)
    ),

    CONSTRAINT check_parts CHECK (
        (parts IS NULL AND parts_count IS NULL)
        OR
        (parts IS NOT NULL AND parts_count IS NOT NULL
            AND array_length(parts, 1) >= 1
            AND array_length(parts, 1) <= 10000
            AND array_length(parts, 1) = parts_count
        )
        OR
        -- object migrated from YDB
        (metadata IS NOT NULL AND (metadata->>'migrated')::bool IS true)
    ),

    CONSTRAINT check_size CHECK (
        (data_size IS NULL)
        OR
        -- single part (simple) object's size is limited by 5GB
        (data_size IS NOT NULL AND parts IS NULL
            AND data_size >= 0
            AND data_size <= 5368709120)
        OR
        -- multipart object's size limit is inherited from s3.object_parts
        (data_size IS NOT NULL AND parts IS NOT NULL
            AND data_size >= 0)
        OR
        -- object migrated from YDB
        (metadata IS NOT NULL AND (metadata->>'migrated')::bool IS true)
    )
) PARTITION BY HASH (bid);

CREATE TABLE s3.objects_noncurrent_0 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 0);
CREATE TABLE s3.objects_noncurrent_1 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 1);
CREATE TABLE s3.objects_noncurrent_2 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 2);
CREATE TABLE s3.objects_noncurrent_3 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 3);
CREATE TABLE s3.objects_noncurrent_4 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 4);
CREATE TABLE s3.objects_noncurrent_5 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 5);
CREATE TABLE s3.objects_noncurrent_6 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 6);
CREATE TABLE s3.objects_noncurrent_7 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 7);
CREATE TABLE s3.objects_noncurrent_8 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 8);
CREATE TABLE s3.objects_noncurrent_9 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 9);
CREATE TABLE s3.objects_noncurrent_10 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 10);
CREATE TABLE s3.objects_noncurrent_11 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 11);
CREATE TABLE s3.objects_noncurrent_12 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 12);
CREATE TABLE s3.objects_noncurrent_13 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 13);
CREATE TABLE s3.objects_noncurrent_14 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 14);
CREATE TABLE s3.objects_noncurrent_15 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 15);
CREATE TABLE s3.objects_noncurrent_16 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 16);
CREATE TABLE s3.objects_noncurrent_17 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 17);
CREATE TABLE s3.objects_noncurrent_18 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 18);
CREATE TABLE s3.objects_noncurrent_19 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 19);
CREATE TABLE s3.objects_noncurrent_20 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 20);
CREATE TABLE s3.objects_noncurrent_21 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 21);
CREATE TABLE s3.objects_noncurrent_22 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 22);
CREATE TABLE s3.objects_noncurrent_23 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 23);
CREATE TABLE s3.objects_noncurrent_24 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 24);
CREATE TABLE s3.objects_noncurrent_25 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 25);
CREATE TABLE s3.objects_noncurrent_26 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 26);
CREATE TABLE s3.objects_noncurrent_27 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 27);
CREATE TABLE s3.objects_noncurrent_28 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 28);
CREATE TABLE s3.objects_noncurrent_29 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 29);
CREATE TABLE s3.objects_noncurrent_30 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 30);
CREATE TABLE s3.objects_noncurrent_31 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 31);
CREATE TABLE s3.objects_noncurrent_32 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 32);
CREATE TABLE s3.objects_noncurrent_33 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 33);
CREATE TABLE s3.objects_noncurrent_34 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 34);
CREATE TABLE s3.objects_noncurrent_35 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 35);
CREATE TABLE s3.objects_noncurrent_36 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 36);
CREATE TABLE s3.objects_noncurrent_37 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 37);
CREATE TABLE s3.objects_noncurrent_38 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 38);
CREATE TABLE s3.objects_noncurrent_39 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 39);
CREATE TABLE s3.objects_noncurrent_40 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 40);
CREATE TABLE s3.objects_noncurrent_41 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 41);
CREATE TABLE s3.objects_noncurrent_42 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 42);
CREATE TABLE s3.objects_noncurrent_43 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 43);
CREATE TABLE s3.objects_noncurrent_44 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 44);
CREATE TABLE s3.objects_noncurrent_45 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 45);
CREATE TABLE s3.objects_noncurrent_46 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 46);
CREATE TABLE s3.objects_noncurrent_47 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 47);
CREATE TABLE s3.objects_noncurrent_48 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 48);
CREATE TABLE s3.objects_noncurrent_49 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 49);
CREATE TABLE s3.objects_noncurrent_50 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 50);
CREATE TABLE s3.objects_noncurrent_51 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 51);
CREATE TABLE s3.objects_noncurrent_52 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 52);
CREATE TABLE s3.objects_noncurrent_53 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 53);
CREATE TABLE s3.objects_noncurrent_54 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 54);
CREATE TABLE s3.objects_noncurrent_55 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 55);
CREATE TABLE s3.objects_noncurrent_56 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 56);
CREATE TABLE s3.objects_noncurrent_57 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 57);
CREATE TABLE s3.objects_noncurrent_58 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 58);
CREATE TABLE s3.objects_noncurrent_59 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 59);
CREATE TABLE s3.objects_noncurrent_60 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 60);
CREATE TABLE s3.objects_noncurrent_61 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 61);
CREATE TABLE s3.objects_noncurrent_62 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 62);
CREATE TABLE s3.objects_noncurrent_63 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 63);
CREATE TABLE s3.objects_noncurrent_64 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 64);
CREATE TABLE s3.objects_noncurrent_65 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 65);
CREATE TABLE s3.objects_noncurrent_66 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 66);
CREATE TABLE s3.objects_noncurrent_67 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 67);
CREATE TABLE s3.objects_noncurrent_68 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 68);
CREATE TABLE s3.objects_noncurrent_69 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 69);
CREATE TABLE s3.objects_noncurrent_70 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 70);
CREATE TABLE s3.objects_noncurrent_71 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 71);
CREATE TABLE s3.objects_noncurrent_72 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 72);
CREATE TABLE s3.objects_noncurrent_73 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 73);
CREATE TABLE s3.objects_noncurrent_74 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 74);
CREATE TABLE s3.objects_noncurrent_75 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 75);
CREATE TABLE s3.objects_noncurrent_76 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 76);
CREATE TABLE s3.objects_noncurrent_77 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 77);
CREATE TABLE s3.objects_noncurrent_78 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 78);
CREATE TABLE s3.objects_noncurrent_79 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 79);
CREATE TABLE s3.objects_noncurrent_80 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 80);
CREATE TABLE s3.objects_noncurrent_81 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 81);
CREATE TABLE s3.objects_noncurrent_82 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 82);
CREATE TABLE s3.objects_noncurrent_83 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 83);
CREATE TABLE s3.objects_noncurrent_84 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 84);
CREATE TABLE s3.objects_noncurrent_85 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 85);
CREATE TABLE s3.objects_noncurrent_86 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 86);
CREATE TABLE s3.objects_noncurrent_87 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 87);
CREATE TABLE s3.objects_noncurrent_88 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 88);
CREATE TABLE s3.objects_noncurrent_89 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 89);
CREATE TABLE s3.objects_noncurrent_90 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 90);
CREATE TABLE s3.objects_noncurrent_91 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 91);
CREATE TABLE s3.objects_noncurrent_92 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 92);
CREATE TABLE s3.objects_noncurrent_93 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 93);
CREATE TABLE s3.objects_noncurrent_94 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 94);
CREATE TABLE s3.objects_noncurrent_95 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 95);
CREATE TABLE s3.objects_noncurrent_96 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 96);
CREATE TABLE s3.objects_noncurrent_97 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 97);
CREATE TABLE s3.objects_noncurrent_98 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 98);
CREATE TABLE s3.objects_noncurrent_99 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 99);
CREATE TABLE s3.objects_noncurrent_100 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 100);
CREATE TABLE s3.objects_noncurrent_101 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 101);
CREATE TABLE s3.objects_noncurrent_102 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 102);
CREATE TABLE s3.objects_noncurrent_103 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 103);
CREATE TABLE s3.objects_noncurrent_104 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 104);
CREATE TABLE s3.objects_noncurrent_105 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 105);
CREATE TABLE s3.objects_noncurrent_106 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 106);
CREATE TABLE s3.objects_noncurrent_107 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 107);
CREATE TABLE s3.objects_noncurrent_108 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 108);
CREATE TABLE s3.objects_noncurrent_109 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 109);
CREATE TABLE s3.objects_noncurrent_110 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 110);
CREATE TABLE s3.objects_noncurrent_111 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 111);
CREATE TABLE s3.objects_noncurrent_112 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 112);
CREATE TABLE s3.objects_noncurrent_113 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 113);
CREATE TABLE s3.objects_noncurrent_114 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 114);
CREATE TABLE s3.objects_noncurrent_115 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 115);
CREATE TABLE s3.objects_noncurrent_116 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 116);
CREATE TABLE s3.objects_noncurrent_117 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 117);
CREATE TABLE s3.objects_noncurrent_118 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 118);
CREATE TABLE s3.objects_noncurrent_119 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 119);
CREATE TABLE s3.objects_noncurrent_120 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 120);
CREATE TABLE s3.objects_noncurrent_121 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 121);
CREATE TABLE s3.objects_noncurrent_122 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 122);
CREATE TABLE s3.objects_noncurrent_123 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 123);
CREATE TABLE s3.objects_noncurrent_124 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 124);
CREATE TABLE s3.objects_noncurrent_125 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 125);
CREATE TABLE s3.objects_noncurrent_126 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 126);
CREATE TABLE s3.objects_noncurrent_127 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 127);
CREATE TABLE s3.objects_noncurrent_128 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 128);
CREATE TABLE s3.objects_noncurrent_129 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 129);
CREATE TABLE s3.objects_noncurrent_130 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 130);
CREATE TABLE s3.objects_noncurrent_131 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 131);
CREATE TABLE s3.objects_noncurrent_132 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 132);
CREATE TABLE s3.objects_noncurrent_133 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 133);
CREATE TABLE s3.objects_noncurrent_134 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 134);
CREATE TABLE s3.objects_noncurrent_135 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 135);
CREATE TABLE s3.objects_noncurrent_136 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 136);
CREATE TABLE s3.objects_noncurrent_137 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 137);
CREATE TABLE s3.objects_noncurrent_138 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 138);
CREATE TABLE s3.objects_noncurrent_139 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 139);
CREATE TABLE s3.objects_noncurrent_140 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 140);
CREATE TABLE s3.objects_noncurrent_141 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 141);
CREATE TABLE s3.objects_noncurrent_142 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 142);
CREATE TABLE s3.objects_noncurrent_143 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 143);
CREATE TABLE s3.objects_noncurrent_144 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 144);
CREATE TABLE s3.objects_noncurrent_145 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 145);
CREATE TABLE s3.objects_noncurrent_146 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 146);
CREATE TABLE s3.objects_noncurrent_147 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 147);
CREATE TABLE s3.objects_noncurrent_148 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 148);
CREATE TABLE s3.objects_noncurrent_149 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 149);
CREATE TABLE s3.objects_noncurrent_150 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 150);
CREATE TABLE s3.objects_noncurrent_151 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 151);
CREATE TABLE s3.objects_noncurrent_152 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 152);
CREATE TABLE s3.objects_noncurrent_153 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 153);
CREATE TABLE s3.objects_noncurrent_154 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 154);
CREATE TABLE s3.objects_noncurrent_155 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 155);
CREATE TABLE s3.objects_noncurrent_156 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 156);
CREATE TABLE s3.objects_noncurrent_157 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 157);
CREATE TABLE s3.objects_noncurrent_158 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 158);
CREATE TABLE s3.objects_noncurrent_159 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 159);
CREATE TABLE s3.objects_noncurrent_160 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 160);
CREATE TABLE s3.objects_noncurrent_161 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 161);
CREATE TABLE s3.objects_noncurrent_162 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 162);
CREATE TABLE s3.objects_noncurrent_163 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 163);
CREATE TABLE s3.objects_noncurrent_164 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 164);
CREATE TABLE s3.objects_noncurrent_165 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 165);
CREATE TABLE s3.objects_noncurrent_166 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 166);
CREATE TABLE s3.objects_noncurrent_167 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 167);
CREATE TABLE s3.objects_noncurrent_168 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 168);
CREATE TABLE s3.objects_noncurrent_169 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 169);
CREATE TABLE s3.objects_noncurrent_170 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 170);
CREATE TABLE s3.objects_noncurrent_171 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 171);
CREATE TABLE s3.objects_noncurrent_172 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 172);
CREATE TABLE s3.objects_noncurrent_173 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 173);
CREATE TABLE s3.objects_noncurrent_174 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 174);
CREATE TABLE s3.objects_noncurrent_175 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 175);
CREATE TABLE s3.objects_noncurrent_176 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 176);
CREATE TABLE s3.objects_noncurrent_177 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 177);
CREATE TABLE s3.objects_noncurrent_178 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 178);
CREATE TABLE s3.objects_noncurrent_179 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 179);
CREATE TABLE s3.objects_noncurrent_180 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 180);
CREATE TABLE s3.objects_noncurrent_181 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 181);
CREATE TABLE s3.objects_noncurrent_182 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 182);
CREATE TABLE s3.objects_noncurrent_183 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 183);
CREATE TABLE s3.objects_noncurrent_184 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 184);
CREATE TABLE s3.objects_noncurrent_185 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 185);
CREATE TABLE s3.objects_noncurrent_186 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 186);
CREATE TABLE s3.objects_noncurrent_187 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 187);
CREATE TABLE s3.objects_noncurrent_188 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 188);
CREATE TABLE s3.objects_noncurrent_189 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 189);
CREATE TABLE s3.objects_noncurrent_190 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 190);
CREATE TABLE s3.objects_noncurrent_191 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 191);
CREATE TABLE s3.objects_noncurrent_192 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 192);
CREATE TABLE s3.objects_noncurrent_193 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 193);
CREATE TABLE s3.objects_noncurrent_194 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 194);
CREATE TABLE s3.objects_noncurrent_195 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 195);
CREATE TABLE s3.objects_noncurrent_196 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 196);
CREATE TABLE s3.objects_noncurrent_197 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 197);
CREATE TABLE s3.objects_noncurrent_198 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 198);
CREATE TABLE s3.objects_noncurrent_199 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 199);
CREATE TABLE s3.objects_noncurrent_200 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 200);
CREATE TABLE s3.objects_noncurrent_201 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 201);
CREATE TABLE s3.objects_noncurrent_202 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 202);
CREATE TABLE s3.objects_noncurrent_203 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 203);
CREATE TABLE s3.objects_noncurrent_204 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 204);
CREATE TABLE s3.objects_noncurrent_205 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 205);
CREATE TABLE s3.objects_noncurrent_206 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 206);
CREATE TABLE s3.objects_noncurrent_207 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 207);
CREATE TABLE s3.objects_noncurrent_208 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 208);
CREATE TABLE s3.objects_noncurrent_209 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 209);
CREATE TABLE s3.objects_noncurrent_210 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 210);
CREATE TABLE s3.objects_noncurrent_211 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 211);
CREATE TABLE s3.objects_noncurrent_212 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 212);
CREATE TABLE s3.objects_noncurrent_213 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 213);
CREATE TABLE s3.objects_noncurrent_214 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 214);
CREATE TABLE s3.objects_noncurrent_215 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 215);
CREATE TABLE s3.objects_noncurrent_216 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 216);
CREATE TABLE s3.objects_noncurrent_217 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 217);
CREATE TABLE s3.objects_noncurrent_218 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 218);
CREATE TABLE s3.objects_noncurrent_219 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 219);
CREATE TABLE s3.objects_noncurrent_220 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 220);
CREATE TABLE s3.objects_noncurrent_221 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 221);
CREATE TABLE s3.objects_noncurrent_222 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 222);
CREATE TABLE s3.objects_noncurrent_223 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 223);
CREATE TABLE s3.objects_noncurrent_224 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 224);
CREATE TABLE s3.objects_noncurrent_225 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 225);
CREATE TABLE s3.objects_noncurrent_226 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 226);
CREATE TABLE s3.objects_noncurrent_227 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 227);
CREATE TABLE s3.objects_noncurrent_228 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 228);
CREATE TABLE s3.objects_noncurrent_229 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 229);
CREATE TABLE s3.objects_noncurrent_230 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 230);
CREATE TABLE s3.objects_noncurrent_231 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 231);
CREATE TABLE s3.objects_noncurrent_232 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 232);
CREATE TABLE s3.objects_noncurrent_233 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 233);
CREATE TABLE s3.objects_noncurrent_234 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 234);
CREATE TABLE s3.objects_noncurrent_235 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 235);
CREATE TABLE s3.objects_noncurrent_236 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 236);
CREATE TABLE s3.objects_noncurrent_237 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 237);
CREATE TABLE s3.objects_noncurrent_238 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 238);
CREATE TABLE s3.objects_noncurrent_239 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 239);
CREATE TABLE s3.objects_noncurrent_240 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 240);
CREATE TABLE s3.objects_noncurrent_241 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 241);
CREATE TABLE s3.objects_noncurrent_242 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 242);
CREATE TABLE s3.objects_noncurrent_243 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 243);
CREATE TABLE s3.objects_noncurrent_244 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 244);
CREATE TABLE s3.objects_noncurrent_245 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 245);
CREATE TABLE s3.objects_noncurrent_246 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 246);
CREATE TABLE s3.objects_noncurrent_247 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 247);
CREATE TABLE s3.objects_noncurrent_248 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 248);
CREATE TABLE s3.objects_noncurrent_249 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 249);
CREATE TABLE s3.objects_noncurrent_250 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 250);
CREATE TABLE s3.objects_noncurrent_251 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 251);
CREATE TABLE s3.objects_noncurrent_252 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 252);
CREATE TABLE s3.objects_noncurrent_253 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 253);
CREATE TABLE s3.objects_noncurrent_254 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 254);
CREATE TABLE s3.objects_noncurrent_255 PARTITION OF s3.objects_noncurrent
    FOR VALUES WITH (MODULUS 256, REMAINDER 255);
