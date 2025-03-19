# -*- coding: utf-8 -*-

from cloud.mdb.salt.salt._modules import mysql_hashes

len_2_pass = {
    1: 'Q',
    5: '9Aq7z',
    10: 'CMunq1PrKH',
    20: 'fZ3nZT8TXRieMNDkMMG9',
    50: '6Rbw3mjQEPl9r8lWpfCVd5RYO1AGpKSFkS5TTf11qWm9JRvqpA',
    100: 'qfVOTeBdXxbBL3f6qcneX3oXKqzQxGW8QgheW4JjgNzuvTsmYW' 'uhCdyN2six4Tv5LmCVH6SB4f0fvWpSlujKuPlIkWZYOV6yy9QA',
    200: 'nSxKFmLRHbyuuaK6o7fukHtZFX8I5P5NU8Br5jMZkaxvRcARfp'
    'iXSWI25bK9QvBsNV0GODiCVVJeqKsMvwHrSfIKX8hfFUbsfwme'
    'tmhaDJOZ2DZibuKCtTHU4wxFJUj2Brekhov4QIyrMZ3IOBUjYk'
    '96c4tTVsPH168cv6duLHk33oJkX7yzI1A1He39mom8A4HK7ZNt',
}

len_2_hash_native = {
    100: "*12B7F01E5178178407089966AFC1A134EDBE9528",
    10: "*FF36B72DA2AB727D4380FCFD39313C295186E876",
    1: "*FB7DA503AFE304D828558DD6C08F3EB169284A3C",
    200: "*B3EA6589A48964AF13AFFFBE6E90386A091E5015",
    20: "*57166FC4BBDF4BFDE892CF3966C30AC3DA76485F",
    50: "*DE491A89661DB180D527516C05C7D07177C234DD",
    5: "*69B6085A06D05476296F257F626D26D3691806C0",
}


len_2_hash_sha2 = {
    100: '''$A$005$Xb_m\x1a\x14\'\x16i\x1aH>\x1dUmO.k1\x11.tBQcU8BapajNqCdLWqlJz2BnO3p2h11hqBxZ1.kB91''',
    10: '''$A$005$&M\x15\x06UEK\rS#{: d}\x1a\x1ak\'Pmqo1L9Dlfn03ogl4DiNubgaRsN4FYJjhE57s08Z39s2''',
    1: '''$A$005$L%*,g1\\\x11Ei%%Y3\x1f\x07uF)56R6OBmao.7gDHAdcEqhrLF33ZjApRNgQyvDHN.tuH45''',
    200: '''$A$005${{qYP.gH:g[zS\t\x05\x0f\x03aNASD77JEQW2A.sYqJkNvEwDCSAmXGXl36z0Lm2puJBMS3''',
    20: '''$A$005$f\'p\x05F\'}\x03\r(O> ]Qf^\x07U.0Tbz2cVYoPpFSeOBoOjtsEOCTDgfcoFN0yu2DEiGA5/''',
    50: '''$A$005$\x12J(fB5*{*["g0@?\x0f\'ei42E0Ds1Agv9r1gMALXRpP7bH.L2Vxwho2CnYQYGPxYd9''',
    5: '''$A$005$9?\x113I\x03c@<~\r"c@g\x15F"z!f4egYDpJtjssix4x7rpd3RIxXlzLK4eeZcST.5SIzc1''',
}


len_2_hash_sha256 = {
    100: '''$5$RS\n|\x1au\x1cksB4\\p+\x0fF]%\x19V$3AHs5aNdC4T.ORmu/THwRjPdfLFqyuotB52nhW5fgp1''',
    10: '''$5$~m\x1f.#wd#\x0bI=r9\x0e#\x12\rg<r$..ZRdIIP/eEPHlUAhTQ1itsFtLIPIcjpDhTUk2NG/F8''',
    1: '''$5$\x1bn\x08;I\\^%-ulR\x7f/\x0f|\r=Y\r$QcN2Q6rtTo.NpYDa7sNqet8BaXuLxhH7GvIMx70ykaC''',
    200: '''$5$qUT`/\n\x11J\x13c\x11(\n\\ElW{"\x14$cX2SM0XEioD0WlOY7befosA4W3SUw2imJy1Ata/W4z7''',
    20: '''$5$\x05f\x0f\tq!\x02DG)8*V#yl, \x11?$Lcuj7UKX3MaFKmeAFD2OZ6qEhK9AoajomfOAYn2kr61''',
    50: '''$5$&^i5%/4\x0f.I8 \x05Kd\x18.C%\x10$ZEicIhnF8oceEbN8IpbyIq3Wr01ntLhtHDzirq763K6''',
    5: '''$5$\x04\n:>\x06}v\x1a\x07\x15%+\x06x\tD%\x01c*$LMOQ4oEQelE47r/ZwBl81O.8AvaTuqFKtpE6XGUBF83''',
}


def test_native_mysql_password():
    for pass_len in len_2_pass:
        my_auth_string = mysql_hashes.get_auth_string(len_2_pass[pass_len], 'mysql_native_password')
        assert len_2_hash_native[pass_len] == my_auth_string


def test_caching_sha2_password():
    for pass_len in len_2_pass:
        my_auth_string = mysql_hashes.get_auth_string(
            len_2_pass[pass_len], 'caching_sha2_password', salt=len_2_hash_sha2[pass_len][7:27]
        )
        assert len_2_hash_sha2[pass_len] == my_auth_string


def test_sha256_password():
    for pass_len in len_2_pass:
        my_auth_string = mysql_hashes.get_auth_string(
            len_2_pass[pass_len], 'sha256_password', salt=len_2_hash_sha256[pass_len][3:23]
        )
        assert len_2_hash_sha256[pass_len] == my_auth_string
