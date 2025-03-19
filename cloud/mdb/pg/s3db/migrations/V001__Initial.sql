CREATE SCHEMA IF NOT EXISTS s3;

CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION btree_gist;

CREATE TYPE s3.bucket_versioning_type AS ENUM (
    'disabled',
    'enabled',
    'suspended'
);

CREATE TYPE s3.object_part AS
(
    part_id         integer,
    created         timestamptz,
    data_size       bigint,
    data_md5        uuid,
    mds_couple_id   integer,
    mds_key_version integer,
    mds_key_uuid    uuid
);

CREATE TABLE s3.objects
(
    bid             uuid             NOT NULL,
    name            text COLLATE "C" NOT NULL,
    created         timestamptz      NOT NULL DEFAULT now(),
    cid             bigint,
    data_size       bigint,
    data_md5        uuid,
    mds_couple_id   integer,
    mds_key_version integer,
    mds_key_uuid    uuid,
    null_version    boolean          NOT NULL DEFAULT true,
    delete_marker   boolean          NOT NULL DEFAULT false,
    parts_count     integer,
    parts           s3.object_part[],
    mds_namespace   text COLLATE "C",
    storage_class   int,
    creator_id      text COLLATE "C",
    metadata        JSONB,
    acl             JSONB,
    CONSTRAINT pk_objects PRIMARY KEY (bid, name),
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
            -- Object migrated from YDB
            (delete_marker IS false
                AND metadata IS NOT NULL AND (metadata ->> 'migrated')::bool IS true
                )
        ),
    CONSTRAINT check_parts CHECK (
            (parts IS NULL AND parts_count IS NULL)
            OR
            (parts IS NOT NULL AND parts_count IS NOT NULL
                AND array_length(parts, 1) >= 1
                AND array_length(parts, 1) <= 10000
                AND array_length(parts, 1) = parts_count)
            OR
            -- object migrated from YDB
            (metadata IS NOT NULL AND (metadata ->> 'migrated')::bool IS true)
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
            (metadata IS NOT NULL AND (metadata ->> 'migrated')::bool IS true)
        )
) PARTITION BY HASH (bid);

CREATE TABLE s3.objects_0 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 0);
CREATE TABLE s3.objects_1 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 1);
CREATE TABLE s3.objects_2 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 2);
CREATE TABLE s3.objects_3 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 3);
CREATE TABLE s3.objects_4 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 4);
CREATE TABLE s3.objects_5 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 5);
CREATE TABLE s3.objects_6 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 6);
CREATE TABLE s3.objects_7 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 7);
CREATE TABLE s3.objects_8 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 8);
CREATE TABLE s3.objects_9 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 9);
CREATE TABLE s3.objects_10 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 10);
CREATE TABLE s3.objects_11 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 11);
CREATE TABLE s3.objects_12 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 12);
CREATE TABLE s3.objects_13 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 13);
CREATE TABLE s3.objects_14 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 14);
CREATE TABLE s3.objects_15 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 15);
CREATE TABLE s3.objects_16 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 16);
CREATE TABLE s3.objects_17 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 17);
CREATE TABLE s3.objects_18 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 18);
CREATE TABLE s3.objects_19 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 19);
CREATE TABLE s3.objects_20 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 20);
CREATE TABLE s3.objects_21 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 21);
CREATE TABLE s3.objects_22 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 22);
CREATE TABLE s3.objects_23 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 23);
CREATE TABLE s3.objects_24 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 24);
CREATE TABLE s3.objects_25 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 25);
CREATE TABLE s3.objects_26 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 26);
CREATE TABLE s3.objects_27 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 27);
CREATE TABLE s3.objects_28 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 28);
CREATE TABLE s3.objects_29 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 29);
CREATE TABLE s3.objects_30 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 30);
CREATE TABLE s3.objects_31 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 31);
CREATE TABLE s3.objects_32 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 32);
CREATE TABLE s3.objects_33 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 33);
CREATE TABLE s3.objects_34 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 34);
CREATE TABLE s3.objects_35 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 35);
CREATE TABLE s3.objects_36 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 36);
CREATE TABLE s3.objects_37 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 37);
CREATE TABLE s3.objects_38 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 38);
CREATE TABLE s3.objects_39 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 39);
CREATE TABLE s3.objects_40 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 40);
CREATE TABLE s3.objects_41 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 41);
CREATE TABLE s3.objects_42 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 42);
CREATE TABLE s3.objects_43 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 43);
CREATE TABLE s3.objects_44 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 44);
CREATE TABLE s3.objects_45 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 45);
CREATE TABLE s3.objects_46 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 46);
CREATE TABLE s3.objects_47 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 47);
CREATE TABLE s3.objects_48 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 48);
CREATE TABLE s3.objects_49 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 49);
CREATE TABLE s3.objects_50 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 50);
CREATE TABLE s3.objects_51 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 51);
CREATE TABLE s3.objects_52 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 52);
CREATE TABLE s3.objects_53 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 53);
CREATE TABLE s3.objects_54 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 54);
CREATE TABLE s3.objects_55 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 55);
CREATE TABLE s3.objects_56 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 56);
CREATE TABLE s3.objects_57 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 57);
CREATE TABLE s3.objects_58 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 58);
CREATE TABLE s3.objects_59 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 59);
CREATE TABLE s3.objects_60 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 60);
CREATE TABLE s3.objects_61 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 61);
CREATE TABLE s3.objects_62 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 62);
CREATE TABLE s3.objects_63 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 63);
CREATE TABLE s3.objects_64 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 64);
CREATE TABLE s3.objects_65 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 65);
CREATE TABLE s3.objects_66 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 66);
CREATE TABLE s3.objects_67 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 67);
CREATE TABLE s3.objects_68 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 68);
CREATE TABLE s3.objects_69 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 69);
CREATE TABLE s3.objects_70 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 70);
CREATE TABLE s3.objects_71 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 71);
CREATE TABLE s3.objects_72 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 72);
CREATE TABLE s3.objects_73 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 73);
CREATE TABLE s3.objects_74 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 74);
CREATE TABLE s3.objects_75 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 75);
CREATE TABLE s3.objects_76 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 76);
CREATE TABLE s3.objects_77 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 77);
CREATE TABLE s3.objects_78 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 78);
CREATE TABLE s3.objects_79 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 79);
CREATE TABLE s3.objects_80 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 80);
CREATE TABLE s3.objects_81 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 81);
CREATE TABLE s3.objects_82 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 82);
CREATE TABLE s3.objects_83 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 83);
CREATE TABLE s3.objects_84 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 84);
CREATE TABLE s3.objects_85 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 85);
CREATE TABLE s3.objects_86 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 86);
CREATE TABLE s3.objects_87 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 87);
CREATE TABLE s3.objects_88 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 88);
CREATE TABLE s3.objects_89 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 89);
CREATE TABLE s3.objects_90 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 90);
CREATE TABLE s3.objects_91 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 91);
CREATE TABLE s3.objects_92 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 92);
CREATE TABLE s3.objects_93 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 93);
CREATE TABLE s3.objects_94 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 94);
CREATE TABLE s3.objects_95 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 95);
CREATE TABLE s3.objects_96 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 96);
CREATE TABLE s3.objects_97 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 97);
CREATE TABLE s3.objects_98 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 98);
CREATE TABLE s3.objects_99 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 99);
CREATE TABLE s3.objects_100 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 100);
CREATE TABLE s3.objects_101 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 101);
CREATE TABLE s3.objects_102 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 102);
CREATE TABLE s3.objects_103 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 103);
CREATE TABLE s3.objects_104 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 104);
CREATE TABLE s3.objects_105 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 105);
CREATE TABLE s3.objects_106 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 106);
CREATE TABLE s3.objects_107 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 107);
CREATE TABLE s3.objects_108 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 108);
CREATE TABLE s3.objects_109 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 109);
CREATE TABLE s3.objects_110 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 110);
CREATE TABLE s3.objects_111 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 111);
CREATE TABLE s3.objects_112 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 112);
CREATE TABLE s3.objects_113 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 113);
CREATE TABLE s3.objects_114 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 114);
CREATE TABLE s3.objects_115 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 115);
CREATE TABLE s3.objects_116 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 116);
CREATE TABLE s3.objects_117 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 117);
CREATE TABLE s3.objects_118 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 118);
CREATE TABLE s3.objects_119 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 119);
CREATE TABLE s3.objects_120 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 120);
CREATE TABLE s3.objects_121 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 121);
CREATE TABLE s3.objects_122 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 122);
CREATE TABLE s3.objects_123 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 123);
CREATE TABLE s3.objects_124 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 124);
CREATE TABLE s3.objects_125 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 125);
CREATE TABLE s3.objects_126 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 126);
CREATE TABLE s3.objects_127 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 127);
CREATE TABLE s3.objects_128 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 128);
CREATE TABLE s3.objects_129 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 129);
CREATE TABLE s3.objects_130 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 130);
CREATE TABLE s3.objects_131 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 131);
CREATE TABLE s3.objects_132 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 132);
CREATE TABLE s3.objects_133 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 133);
CREATE TABLE s3.objects_134 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 134);
CREATE TABLE s3.objects_135 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 135);
CREATE TABLE s3.objects_136 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 136);
CREATE TABLE s3.objects_137 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 137);
CREATE TABLE s3.objects_138 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 138);
CREATE TABLE s3.objects_139 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 139);
CREATE TABLE s3.objects_140 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 140);
CREATE TABLE s3.objects_141 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 141);
CREATE TABLE s3.objects_142 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 142);
CREATE TABLE s3.objects_143 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 143);
CREATE TABLE s3.objects_144 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 144);
CREATE TABLE s3.objects_145 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 145);
CREATE TABLE s3.objects_146 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 146);
CREATE TABLE s3.objects_147 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 147);
CREATE TABLE s3.objects_148 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 148);
CREATE TABLE s3.objects_149 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 149);
CREATE TABLE s3.objects_150 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 150);
CREATE TABLE s3.objects_151 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 151);
CREATE TABLE s3.objects_152 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 152);
CREATE TABLE s3.objects_153 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 153);
CREATE TABLE s3.objects_154 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 154);
CREATE TABLE s3.objects_155 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 155);
CREATE TABLE s3.objects_156 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 156);
CREATE TABLE s3.objects_157 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 157);
CREATE TABLE s3.objects_158 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 158);
CREATE TABLE s3.objects_159 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 159);
CREATE TABLE s3.objects_160 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 160);
CREATE TABLE s3.objects_161 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 161);
CREATE TABLE s3.objects_162 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 162);
CREATE TABLE s3.objects_163 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 163);
CREATE TABLE s3.objects_164 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 164);
CREATE TABLE s3.objects_165 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 165);
CREATE TABLE s3.objects_166 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 166);
CREATE TABLE s3.objects_167 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 167);
CREATE TABLE s3.objects_168 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 168);
CREATE TABLE s3.objects_169 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 169);
CREATE TABLE s3.objects_170 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 170);
CREATE TABLE s3.objects_171 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 171);
CREATE TABLE s3.objects_172 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 172);
CREATE TABLE s3.objects_173 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 173);
CREATE TABLE s3.objects_174 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 174);
CREATE TABLE s3.objects_175 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 175);
CREATE TABLE s3.objects_176 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 176);
CREATE TABLE s3.objects_177 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 177);
CREATE TABLE s3.objects_178 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 178);
CREATE TABLE s3.objects_179 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 179);
CREATE TABLE s3.objects_180 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 180);
CREATE TABLE s3.objects_181 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 181);
CREATE TABLE s3.objects_182 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 182);
CREATE TABLE s3.objects_183 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 183);
CREATE TABLE s3.objects_184 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 184);
CREATE TABLE s3.objects_185 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 185);
CREATE TABLE s3.objects_186 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 186);
CREATE TABLE s3.objects_187 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 187);
CREATE TABLE s3.objects_188 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 188);
CREATE TABLE s3.objects_189 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 189);
CREATE TABLE s3.objects_190 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 190);
CREATE TABLE s3.objects_191 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 191);
CREATE TABLE s3.objects_192 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 192);
CREATE TABLE s3.objects_193 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 193);
CREATE TABLE s3.objects_194 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 194);
CREATE TABLE s3.objects_195 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 195);
CREATE TABLE s3.objects_196 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 196);
CREATE TABLE s3.objects_197 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 197);
CREATE TABLE s3.objects_198 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 198);
CREATE TABLE s3.objects_199 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 199);
CREATE TABLE s3.objects_200 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 200);
CREATE TABLE s3.objects_201 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 201);
CREATE TABLE s3.objects_202 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 202);
CREATE TABLE s3.objects_203 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 203);
CREATE TABLE s3.objects_204 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 204);
CREATE TABLE s3.objects_205 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 205);
CREATE TABLE s3.objects_206 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 206);
CREATE TABLE s3.objects_207 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 207);
CREATE TABLE s3.objects_208 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 208);
CREATE TABLE s3.objects_209 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 209);
CREATE TABLE s3.objects_210 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 210);
CREATE TABLE s3.objects_211 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 211);
CREATE TABLE s3.objects_212 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 212);
CREATE TABLE s3.objects_213 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 213);
CREATE TABLE s3.objects_214 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 214);
CREATE TABLE s3.objects_215 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 215);
CREATE TABLE s3.objects_216 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 216);
CREATE TABLE s3.objects_217 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 217);
CREATE TABLE s3.objects_218 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 218);
CREATE TABLE s3.objects_219 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 219);
CREATE TABLE s3.objects_220 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 220);
CREATE TABLE s3.objects_221 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 221);
CREATE TABLE s3.objects_222 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 222);
CREATE TABLE s3.objects_223 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 223);
CREATE TABLE s3.objects_224 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 224);
CREATE TABLE s3.objects_225 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 225);
CREATE TABLE s3.objects_226 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 226);
CREATE TABLE s3.objects_227 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 227);
CREATE TABLE s3.objects_228 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 228);
CREATE TABLE s3.objects_229 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 229);
CREATE TABLE s3.objects_230 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 230);
CREATE TABLE s3.objects_231 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 231);
CREATE TABLE s3.objects_232 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 232);
CREATE TABLE s3.objects_233 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 233);
CREATE TABLE s3.objects_234 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 234);
CREATE TABLE s3.objects_235 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 235);
CREATE TABLE s3.objects_236 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 236);
CREATE TABLE s3.objects_237 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 237);
CREATE TABLE s3.objects_238 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 238);
CREATE TABLE s3.objects_239 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 239);
CREATE TABLE s3.objects_240 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 240);
CREATE TABLE s3.objects_241 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 241);
CREATE TABLE s3.objects_242 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 242);
CREATE TABLE s3.objects_243 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 243);
CREATE TABLE s3.objects_244 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 244);
CREATE TABLE s3.objects_245 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 245);
CREATE TABLE s3.objects_246 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 246);
CREATE TABLE s3.objects_247 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 247);
CREATE TABLE s3.objects_248 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 248);
CREATE TABLE s3.objects_249 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 249);
CREATE TABLE s3.objects_250 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 250);
CREATE TABLE s3.objects_251 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 251);
CREATE TABLE s3.objects_252 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 252);
CREATE TABLE s3.objects_253 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 253);
CREATE TABLE s3.objects_254 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 254);
CREATE TABLE s3.objects_255 PARTITION OF s3.objects
    FOR VALUES WITH (MODULUS 256, REMAINDER 255);

CREATE TABLE s3.object_parts
(
    bid             uuid                      NOT NULL,
    cid             bigint,
    name            text COLLATE "C"          NOT NULL,
    object_created  timestamptz               NOT NULL,
    part_id         integer                   NOT NULL,
    created         timestamptz DEFAULT now() NOT NULL,
    data_size       bigint      DEFAULT 0     NOT NULL,
    data_md5        uuid,
    mds_couple_id   integer,
    mds_key_version integer,
    mds_key_uuid    uuid,
    mds_namespace   text COLLATE "C",
    storage_class   integer,
    creator_id      text COLLATE "C",
    metadata        jsonb,
    acl             jsonb,
    CONSTRAINT check_data CHECK (
        -- "Root" record with no "metadata" record in MDS
            (part_id = 0 AND mds_couple_id IS NULL
                AND mds_key_version IS NULL AND mds_key_uuid IS NULL
                AND data_md5 IS NULL)
            OR
            -- "Root" record with "metadata" record in MDS
            (part_id = 0 AND mds_couple_id IS NOT NULL
                AND mds_key_version IS NOT NULL AND mds_key_uuid IS NOT NULL
                AND data_md5 IS NULL)
            OR
            (part_id > 0 AND part_id <= 10000 AND mds_couple_id IS NOT NULL
                AND mds_key_version IS NOT NULL AND mds_key_uuid IS NOT NULL
                AND data_md5 IS NOT NULL)
        ),
    CONSTRAINT check_size CHECK (
            (part_id = 0 AND data_size >= 0)
            OR
            (part_id > 0 AND data_size >= 0 AND data_size <= 5368709120) -- 5GB
        )
);

CREATE UNIQUE INDEX pk_object_parts ON s3.object_parts
    (bid, name, object_created, part_id, created);

CREATE TABLE s3.chunks_counters_queue
(
    id                             serial                    NOT NULL,
    bid                            uuid                      NOT NULL,
    cid                            bigint                    NOT NULL,
    simple_objects_count_change    bigint      DEFAULT 0     NOT NULL,
    simple_objects_size_change     bigint      DEFAULT 0     NOT NULL,
    multipart_objects_count_change bigint      DEFAULT 0     NOT NULL,
    multipart_objects_size_change  bigint      DEFAULT 0     NOT NULL,
    objects_parts_count_change     bigint      DEFAULT 0     NOT NULL,
    objects_parts_size_change      bigint      DEFAULT 0     NOT NULL,
    created_ts                     timestamptz DEFAULT now() NOT NULL,
    storage_class                  integer,
    deleted_objects_count_change   bigint      DEFAULT 0     NOT NULL,
    deleted_objects_size_change    bigint      DEFAULT 0     NOT NULL,
    active_multipart_count_change  bigint      DEFAULT 0     NOT NULL,
    CONSTRAINT pk_chunks_counters_queue PRIMARY KEY (id)
);

CREATE INDEX i_chunks_counters_queue_bid_cid ON s3.chunks_counters_queue USING btree (bid, cid);

ALTER SEQUENCE s3.chunks_counters_queue_id_seq
    MAXVALUE 2147483647
        CYCLE;

CREATE TYPE s3.keyrange AS RANGE
(
    SUBTYPE = text,
    COLLATION =
    "C"
);

CREATE OR REPLACE FUNCTION s3.to_keyrange(start_key text, end_key text)
    RETURNS s3.keyrange
    LANGUAGE plpgsql
    IMMUTABLE
AS
$function$
BEGIN
    IF start_key IS NULL AND end_key IS NULL THEN
        RETURN '[,)'::s3.keyrange;
    ELSIF start_key IS NULL THEN
        RETURN format('[,%I)', end_key)::s3.keyrange;
    ELSIF end_key IS NULL THEN
        RETURN format('[%I,)', start_key)::s3.keyrange;
    ELSE
        RETURN format('[%I,%I)', start_key, end_key)::s3.keyrange;
    END IF;
END;
$function$;

CREATE TABLE s3.chunks
(
    bid                     uuid             NOT NULL,
    cid                     bigint           NOT NULL,
    simple_objects_count    bigint DEFAULT 0 NOT NULL,
    simple_objects_size     bigint DEFAULT 0 NOT NULL,
    multipart_objects_count bigint DEFAULT 0 NOT NULL,
    multipart_objects_size  bigint DEFAULT 0 NOT NULL,
    objects_parts_count     bigint DEFAULT 0 NOT NULL,
    objects_parts_size      bigint DEFAULT 0 NOT NULL,
    start_key               text COLLATE "C",
    end_key                 text COLLATE "C",
    updated_ts              timestamptz      NOT NULL DEFAULT now(),
    CONSTRAINT pk_chunks PRIMARY KEY (bid, cid),
    CONSTRAINT check_key_range_empty CHECK ((NOT isempty(s3.to_keyrange(start_key, end_key)))),
    CONSTRAINT idx_exclude_bid_key_range EXCLUDE USING gist( (bid::text) WITH =, s3.to_keyrange(
            start_key, end_key) WITH &&) DEFERRABLE INITIALLY DEFERRED
);

CREATE INDEX idx_bid_start_key ON s3.chunks
    USING btree (bid, coalesce(start_key, '') DESC);

INSERT INTO s3.chunks(bid, cid)
VALUES (uuid_nil(), -1);

CREATE OR REPLACE FUNCTION s3.chunk_check_empty() RETURNS TRIGGER
    LANGUAGE plpgsql AS
$function$
DECLARE
    v_keyrange s3.keyrange;
BEGIN
    IF TG_OP = 'UPDATE' THEN
        IF NEW.end_key <> OLD.end_key THEN
            PERFORM true
            FROM s3.chunks
                 -- Use exclude constraint gist index (bid::text, s3.to_keyrange)
            WHERE bid::text = NEW.bid::text
              AND s3.to_keyrange(start_key, end_key) = s3.to_keyrange(NEW.end_key, OLD.end_key);
            IF NOT FOUND THEN
                RAISE EXCEPTION 'Incorrect update keys of chunk (%, %, %, %)'
                    ': chunk (%, %) not found', OLD.bid, OLD.cid,
                    OLD.start_key, OLD.end_key, NEW.end_key, OLD.end_key;
            END IF;
        END IF;

        IF NEW.start_key <> OLD.start_key THEN
            PERFORM true
            FROM s3.chunks
                 -- Use exclude constraint gist index (bid::text, s3.to_keyrange)
            WHERE bid::text = NEW.bid::text
              AND s3.to_keyrange(start_key, end_key) = s3.to_keyrange(OLD.start_key, NEW.start_key);
            IF NOT FOUND THEN
                RAISE EXCEPTION 'Incorrect update keys of chunk (%, %, %, %)'
                    ': chunk (%, %) not found', OLD.bid, OLD.cid,
                    OLD.start_key, OLD.end_key, OLD.start_key, NEW.start_key;
            END IF;
        END IF;
    ELSIF TG_OP = 'DELETE' THEN

        RAISE NOTICE 'Checking emptiness for chunk (%, %, %, %)',
            OLD.bid, OLD.cid, OLD.start_key, OLD.end_key;
        /*
         * We do not need to do anything about locking to prevent
         * race conditions with insert into s3.objects and delete
         * from s3.chunks here. Because any insert into s3.objects
         * obtain share lock on s3.chunks row and here s3.chunks
         * row already locked.
         */
        EXECUTE format($$
            SELECT true FROM s3.objects
                WHERE bid = %1$L /* bid */
                    AND (%2$L IS NULL OR name >= %2$L) /* start_key */
                    AND (%3$L IS NULL OR name < %3$L) /* end_key */
            LIMIT 1
        $$, OLD.bid, OLD.start_key, OLD.end_key)
            INTO FOUND;
        IF FOUND THEN
            RAISE EXCEPTION 'Chunk (%, %, %, %) is not empty',
                OLD.bid, OLD.cid, OLD.start_key, OLD.end_key
                USING ERRCODE = 23503;
        END IF;
    ELSE
        RAISE EXCEPTION 'This funciton must be called only on DELETE or UPDATE';
    END IF;

    RETURN OLD;
END;
$function$;

CREATE CONSTRAINT TRIGGER tg_chunk_check_empty
    AFTER DELETE OR UPDATE OF start_key, end_key
    ON s3.chunks
    DEFERRABLE INITIALLY DEFERRED
    FOR EACH ROW
EXECUTE PROCEDURE s3.chunk_check_empty();

CREATE TABLE s3.storage_delete_queue
(
    bid             uuid             NOT NULL,
    name            text COLLATE "C" NOT NULL,
    part_id         integer,
    data_size       bigint                    DEFAULT 0 NOT NULL,
    mds_couple_id   integer          NOT NULL,
    mds_key_version integer          NOT NULL,
    mds_key_uuid    uuid             NOT NULL,
    remove_after_ts timestamptz      NOT NULL DEFAULT current_timestamp,
    deleted_ts      timestamptz      NOT NULL DEFAULT current_timestamp,
    created         timestamptz      NOT NULL,
    data_md5        uuid,
    parts_count     integer,
    mds_namespace   text COLLATE "C",
    storage_class   integer,
    creator_id      text COLLATE "C",
    metadata        jsonb,
    acl             jsonb,
    CONSTRAINT check_part_id CHECK (((part_id IS NULL) OR ((part_id >= 0) AND (part_id <= 10000))))
);

CREATE INDEX i_storage_delete_queue_bid_ts ON s3.storage_delete_queue USING btree (bid, deleted_ts, remove_after_ts);
CREATE INDEX i_storage_delete_queue_ts ON s3.storage_delete_queue USING btree (deleted_ts, remove_after_ts);
CREATE UNIQUE INDEX pk_delete_queue ON s3.storage_delete_queue (
                                                                bid, name, COALESCE(part_id, 0), mds_couple_id,
                                                                mds_key_version, mds_key_uuid);
