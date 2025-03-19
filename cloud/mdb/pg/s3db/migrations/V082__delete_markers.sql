CREATE TABLE s3.object_delete_markers
(
    bid              uuid    NOT NULL,
    name             text COLLATE "C" NOT NULL,
    created          timestamp with time zone NOT NULL DEFAULT current_timestamp,
    creator_id       text COLLATE "C",
    null_version     boolean NOT NULL DEFAULT true,

    CONSTRAINT pk_object_delete_markers PRIMARY KEY (bid, name),

    CONSTRAINT check_object_name CHECK (
        int4range(1, 1025) @> octet_length(convert_to(name, 'UTF8'))
    )
) PARTITION BY HASH (bid);

CREATE TABLE s3.object_delete_markers_0 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 0);
CREATE TABLE s3.object_delete_markers_1 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 1);
CREATE TABLE s3.object_delete_markers_2 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 2);
CREATE TABLE s3.object_delete_markers_3 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 3);
CREATE TABLE s3.object_delete_markers_4 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 4);
CREATE TABLE s3.object_delete_markers_5 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 5);
CREATE TABLE s3.object_delete_markers_6 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 6);
CREATE TABLE s3.object_delete_markers_7 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 7);
CREATE TABLE s3.object_delete_markers_8 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 8);
CREATE TABLE s3.object_delete_markers_9 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 9);
CREATE TABLE s3.object_delete_markers_10 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 10);
CREATE TABLE s3.object_delete_markers_11 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 11);
CREATE TABLE s3.object_delete_markers_12 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 12);
CREATE TABLE s3.object_delete_markers_13 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 13);
CREATE TABLE s3.object_delete_markers_14 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 14);
CREATE TABLE s3.object_delete_markers_15 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 15);
CREATE TABLE s3.object_delete_markers_16 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 16);
CREATE TABLE s3.object_delete_markers_17 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 17);
CREATE TABLE s3.object_delete_markers_18 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 18);
CREATE TABLE s3.object_delete_markers_19 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 19);
CREATE TABLE s3.object_delete_markers_20 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 20);
CREATE TABLE s3.object_delete_markers_21 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 21);
CREATE TABLE s3.object_delete_markers_22 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 22);
CREATE TABLE s3.object_delete_markers_23 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 23);
CREATE TABLE s3.object_delete_markers_24 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 24);
CREATE TABLE s3.object_delete_markers_25 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 25);
CREATE TABLE s3.object_delete_markers_26 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 26);
CREATE TABLE s3.object_delete_markers_27 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 27);
CREATE TABLE s3.object_delete_markers_28 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 28);
CREATE TABLE s3.object_delete_markers_29 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 29);
CREATE TABLE s3.object_delete_markers_30 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 30);
CREATE TABLE s3.object_delete_markers_31 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 31);
CREATE TABLE s3.object_delete_markers_32 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 32);
CREATE TABLE s3.object_delete_markers_33 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 33);
CREATE TABLE s3.object_delete_markers_34 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 34);
CREATE TABLE s3.object_delete_markers_35 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 35);
CREATE TABLE s3.object_delete_markers_36 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 36);
CREATE TABLE s3.object_delete_markers_37 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 37);
CREATE TABLE s3.object_delete_markers_38 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 38);
CREATE TABLE s3.object_delete_markers_39 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 39);
CREATE TABLE s3.object_delete_markers_40 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 40);
CREATE TABLE s3.object_delete_markers_41 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 41);
CREATE TABLE s3.object_delete_markers_42 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 42);
CREATE TABLE s3.object_delete_markers_43 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 43);
CREATE TABLE s3.object_delete_markers_44 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 44);
CREATE TABLE s3.object_delete_markers_45 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 45);
CREATE TABLE s3.object_delete_markers_46 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 46);
CREATE TABLE s3.object_delete_markers_47 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 47);
CREATE TABLE s3.object_delete_markers_48 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 48);
CREATE TABLE s3.object_delete_markers_49 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 49);
CREATE TABLE s3.object_delete_markers_50 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 50);
CREATE TABLE s3.object_delete_markers_51 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 51);
CREATE TABLE s3.object_delete_markers_52 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 52);
CREATE TABLE s3.object_delete_markers_53 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 53);
CREATE TABLE s3.object_delete_markers_54 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 54);
CREATE TABLE s3.object_delete_markers_55 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 55);
CREATE TABLE s3.object_delete_markers_56 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 56);
CREATE TABLE s3.object_delete_markers_57 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 57);
CREATE TABLE s3.object_delete_markers_58 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 58);
CREATE TABLE s3.object_delete_markers_59 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 59);
CREATE TABLE s3.object_delete_markers_60 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 60);
CREATE TABLE s3.object_delete_markers_61 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 61);
CREATE TABLE s3.object_delete_markers_62 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 62);
CREATE TABLE s3.object_delete_markers_63 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 63);
CREATE TABLE s3.object_delete_markers_64 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 64);
CREATE TABLE s3.object_delete_markers_65 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 65);
CREATE TABLE s3.object_delete_markers_66 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 66);
CREATE TABLE s3.object_delete_markers_67 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 67);
CREATE TABLE s3.object_delete_markers_68 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 68);
CREATE TABLE s3.object_delete_markers_69 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 69);
CREATE TABLE s3.object_delete_markers_70 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 70);
CREATE TABLE s3.object_delete_markers_71 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 71);
CREATE TABLE s3.object_delete_markers_72 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 72);
CREATE TABLE s3.object_delete_markers_73 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 73);
CREATE TABLE s3.object_delete_markers_74 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 74);
CREATE TABLE s3.object_delete_markers_75 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 75);
CREATE TABLE s3.object_delete_markers_76 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 76);
CREATE TABLE s3.object_delete_markers_77 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 77);
CREATE TABLE s3.object_delete_markers_78 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 78);
CREATE TABLE s3.object_delete_markers_79 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 79);
CREATE TABLE s3.object_delete_markers_80 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 80);
CREATE TABLE s3.object_delete_markers_81 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 81);
CREATE TABLE s3.object_delete_markers_82 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 82);
CREATE TABLE s3.object_delete_markers_83 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 83);
CREATE TABLE s3.object_delete_markers_84 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 84);
CREATE TABLE s3.object_delete_markers_85 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 85);
CREATE TABLE s3.object_delete_markers_86 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 86);
CREATE TABLE s3.object_delete_markers_87 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 87);
CREATE TABLE s3.object_delete_markers_88 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 88);
CREATE TABLE s3.object_delete_markers_89 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 89);
CREATE TABLE s3.object_delete_markers_90 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 90);
CREATE TABLE s3.object_delete_markers_91 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 91);
CREATE TABLE s3.object_delete_markers_92 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 92);
CREATE TABLE s3.object_delete_markers_93 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 93);
CREATE TABLE s3.object_delete_markers_94 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 94);
CREATE TABLE s3.object_delete_markers_95 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 95);
CREATE TABLE s3.object_delete_markers_96 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 96);
CREATE TABLE s3.object_delete_markers_97 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 97);
CREATE TABLE s3.object_delete_markers_98 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 98);
CREATE TABLE s3.object_delete_markers_99 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 99);
CREATE TABLE s3.object_delete_markers_100 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 100);
CREATE TABLE s3.object_delete_markers_101 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 101);
CREATE TABLE s3.object_delete_markers_102 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 102);
CREATE TABLE s3.object_delete_markers_103 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 103);
CREATE TABLE s3.object_delete_markers_104 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 104);
CREATE TABLE s3.object_delete_markers_105 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 105);
CREATE TABLE s3.object_delete_markers_106 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 106);
CREATE TABLE s3.object_delete_markers_107 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 107);
CREATE TABLE s3.object_delete_markers_108 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 108);
CREATE TABLE s3.object_delete_markers_109 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 109);
CREATE TABLE s3.object_delete_markers_110 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 110);
CREATE TABLE s3.object_delete_markers_111 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 111);
CREATE TABLE s3.object_delete_markers_112 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 112);
CREATE TABLE s3.object_delete_markers_113 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 113);
CREATE TABLE s3.object_delete_markers_114 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 114);
CREATE TABLE s3.object_delete_markers_115 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 115);
CREATE TABLE s3.object_delete_markers_116 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 116);
CREATE TABLE s3.object_delete_markers_117 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 117);
CREATE TABLE s3.object_delete_markers_118 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 118);
CREATE TABLE s3.object_delete_markers_119 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 119);
CREATE TABLE s3.object_delete_markers_120 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 120);
CREATE TABLE s3.object_delete_markers_121 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 121);
CREATE TABLE s3.object_delete_markers_122 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 122);
CREATE TABLE s3.object_delete_markers_123 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 123);
CREATE TABLE s3.object_delete_markers_124 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 124);
CREATE TABLE s3.object_delete_markers_125 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 125);
CREATE TABLE s3.object_delete_markers_126 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 126);
CREATE TABLE s3.object_delete_markers_127 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 127);
CREATE TABLE s3.object_delete_markers_128 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 128);
CREATE TABLE s3.object_delete_markers_129 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 129);
CREATE TABLE s3.object_delete_markers_130 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 130);
CREATE TABLE s3.object_delete_markers_131 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 131);
CREATE TABLE s3.object_delete_markers_132 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 132);
CREATE TABLE s3.object_delete_markers_133 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 133);
CREATE TABLE s3.object_delete_markers_134 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 134);
CREATE TABLE s3.object_delete_markers_135 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 135);
CREATE TABLE s3.object_delete_markers_136 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 136);
CREATE TABLE s3.object_delete_markers_137 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 137);
CREATE TABLE s3.object_delete_markers_138 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 138);
CREATE TABLE s3.object_delete_markers_139 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 139);
CREATE TABLE s3.object_delete_markers_140 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 140);
CREATE TABLE s3.object_delete_markers_141 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 141);
CREATE TABLE s3.object_delete_markers_142 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 142);
CREATE TABLE s3.object_delete_markers_143 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 143);
CREATE TABLE s3.object_delete_markers_144 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 144);
CREATE TABLE s3.object_delete_markers_145 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 145);
CREATE TABLE s3.object_delete_markers_146 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 146);
CREATE TABLE s3.object_delete_markers_147 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 147);
CREATE TABLE s3.object_delete_markers_148 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 148);
CREATE TABLE s3.object_delete_markers_149 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 149);
CREATE TABLE s3.object_delete_markers_150 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 150);
CREATE TABLE s3.object_delete_markers_151 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 151);
CREATE TABLE s3.object_delete_markers_152 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 152);
CREATE TABLE s3.object_delete_markers_153 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 153);
CREATE TABLE s3.object_delete_markers_154 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 154);
CREATE TABLE s3.object_delete_markers_155 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 155);
CREATE TABLE s3.object_delete_markers_156 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 156);
CREATE TABLE s3.object_delete_markers_157 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 157);
CREATE TABLE s3.object_delete_markers_158 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 158);
CREATE TABLE s3.object_delete_markers_159 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 159);
CREATE TABLE s3.object_delete_markers_160 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 160);
CREATE TABLE s3.object_delete_markers_161 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 161);
CREATE TABLE s3.object_delete_markers_162 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 162);
CREATE TABLE s3.object_delete_markers_163 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 163);
CREATE TABLE s3.object_delete_markers_164 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 164);
CREATE TABLE s3.object_delete_markers_165 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 165);
CREATE TABLE s3.object_delete_markers_166 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 166);
CREATE TABLE s3.object_delete_markers_167 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 167);
CREATE TABLE s3.object_delete_markers_168 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 168);
CREATE TABLE s3.object_delete_markers_169 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 169);
CREATE TABLE s3.object_delete_markers_170 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 170);
CREATE TABLE s3.object_delete_markers_171 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 171);
CREATE TABLE s3.object_delete_markers_172 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 172);
CREATE TABLE s3.object_delete_markers_173 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 173);
CREATE TABLE s3.object_delete_markers_174 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 174);
CREATE TABLE s3.object_delete_markers_175 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 175);
CREATE TABLE s3.object_delete_markers_176 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 176);
CREATE TABLE s3.object_delete_markers_177 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 177);
CREATE TABLE s3.object_delete_markers_178 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 178);
CREATE TABLE s3.object_delete_markers_179 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 179);
CREATE TABLE s3.object_delete_markers_180 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 180);
CREATE TABLE s3.object_delete_markers_181 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 181);
CREATE TABLE s3.object_delete_markers_182 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 182);
CREATE TABLE s3.object_delete_markers_183 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 183);
CREATE TABLE s3.object_delete_markers_184 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 184);
CREATE TABLE s3.object_delete_markers_185 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 185);
CREATE TABLE s3.object_delete_markers_186 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 186);
CREATE TABLE s3.object_delete_markers_187 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 187);
CREATE TABLE s3.object_delete_markers_188 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 188);
CREATE TABLE s3.object_delete_markers_189 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 189);
CREATE TABLE s3.object_delete_markers_190 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 190);
CREATE TABLE s3.object_delete_markers_191 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 191);
CREATE TABLE s3.object_delete_markers_192 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 192);
CREATE TABLE s3.object_delete_markers_193 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 193);
CREATE TABLE s3.object_delete_markers_194 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 194);
CREATE TABLE s3.object_delete_markers_195 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 195);
CREATE TABLE s3.object_delete_markers_196 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 196);
CREATE TABLE s3.object_delete_markers_197 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 197);
CREATE TABLE s3.object_delete_markers_198 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 198);
CREATE TABLE s3.object_delete_markers_199 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 199);
CREATE TABLE s3.object_delete_markers_200 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 200);
CREATE TABLE s3.object_delete_markers_201 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 201);
CREATE TABLE s3.object_delete_markers_202 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 202);
CREATE TABLE s3.object_delete_markers_203 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 203);
CREATE TABLE s3.object_delete_markers_204 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 204);
CREATE TABLE s3.object_delete_markers_205 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 205);
CREATE TABLE s3.object_delete_markers_206 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 206);
CREATE TABLE s3.object_delete_markers_207 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 207);
CREATE TABLE s3.object_delete_markers_208 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 208);
CREATE TABLE s3.object_delete_markers_209 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 209);
CREATE TABLE s3.object_delete_markers_210 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 210);
CREATE TABLE s3.object_delete_markers_211 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 211);
CREATE TABLE s3.object_delete_markers_212 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 212);
CREATE TABLE s3.object_delete_markers_213 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 213);
CREATE TABLE s3.object_delete_markers_214 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 214);
CREATE TABLE s3.object_delete_markers_215 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 215);
CREATE TABLE s3.object_delete_markers_216 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 216);
CREATE TABLE s3.object_delete_markers_217 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 217);
CREATE TABLE s3.object_delete_markers_218 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 218);
CREATE TABLE s3.object_delete_markers_219 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 219);
CREATE TABLE s3.object_delete_markers_220 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 220);
CREATE TABLE s3.object_delete_markers_221 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 221);
CREATE TABLE s3.object_delete_markers_222 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 222);
CREATE TABLE s3.object_delete_markers_223 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 223);
CREATE TABLE s3.object_delete_markers_224 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 224);
CREATE TABLE s3.object_delete_markers_225 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 225);
CREATE TABLE s3.object_delete_markers_226 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 226);
CREATE TABLE s3.object_delete_markers_227 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 227);
CREATE TABLE s3.object_delete_markers_228 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 228);
CREATE TABLE s3.object_delete_markers_229 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 229);
CREATE TABLE s3.object_delete_markers_230 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 230);
CREATE TABLE s3.object_delete_markers_231 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 231);
CREATE TABLE s3.object_delete_markers_232 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 232);
CREATE TABLE s3.object_delete_markers_233 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 233);
CREATE TABLE s3.object_delete_markers_234 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 234);
CREATE TABLE s3.object_delete_markers_235 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 235);
CREATE TABLE s3.object_delete_markers_236 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 236);
CREATE TABLE s3.object_delete_markers_237 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 237);
CREATE TABLE s3.object_delete_markers_238 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 238);
CREATE TABLE s3.object_delete_markers_239 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 239);
CREATE TABLE s3.object_delete_markers_240 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 240);
CREATE TABLE s3.object_delete_markers_241 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 241);
CREATE TABLE s3.object_delete_markers_242 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 242);
CREATE TABLE s3.object_delete_markers_243 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 243);
CREATE TABLE s3.object_delete_markers_244 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 244);
CREATE TABLE s3.object_delete_markers_245 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 245);
CREATE TABLE s3.object_delete_markers_246 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 246);
CREATE TABLE s3.object_delete_markers_247 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 247);
CREATE TABLE s3.object_delete_markers_248 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 248);
CREATE TABLE s3.object_delete_markers_249 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 249);
CREATE TABLE s3.object_delete_markers_250 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 250);
CREATE TABLE s3.object_delete_markers_251 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 251);
CREATE TABLE s3.object_delete_markers_252 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 252);
CREATE TABLE s3.object_delete_markers_253 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 253);
CREATE TABLE s3.object_delete_markers_254 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 254);
CREATE TABLE s3.object_delete_markers_255 PARTITION OF s3.object_delete_markers
    FOR VALUES WITH (MODULUS 256, REMAINDER 255);
