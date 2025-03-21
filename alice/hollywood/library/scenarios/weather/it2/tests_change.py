import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice

BG_GRANET_SOURCE_TEXT = (
    'bg_granet_source_text=H4sIAAAAAAAAA8196XKbR5LgqzBoOYaUbPXMzs7GrmNjGLLc3e4YXyOp29'
    'EhKxQQ-UnEGAQ4AChZM5gIkbQseSVL2_tn99--AkgRIsQDfAXgjbbyqKrMrCqAkuCOZdttfFlX1pWVlZ'
    'XHvy_ea9eaVff23VZ7o9a9fb9qd-qt5uIni3-3-NFio9a85362t9zvjVodwPdc3gdVrbtetW-vrrv06v'
    'K9drPrMmzWOt_XG43O4id3a41O9dFip1n9m8tRC4Bu9UPXJf_74mprY6PV_E1t7V-q1W79ftWhOj65uV'
    'jf2Gy1u58shBwOoTsd3wT9c-EqJl7-cqtTX601PnGgBff3YaPa2Kjxx_hksjs-nDwbH4374-PJ8_Gp-_'
    '0mqeL6OrSWq-DDqlm706hudx42W82HG52FWqPhk9bqHZ3mfnSqLieHujdiEYfGaPLI_bszHjhUnrvvIa'
    'JjEj1YorhRmzuKrpVTROXY_W8wPprsuNHS4wTZHFJ9N4i74xMNlgCB6VfVg3njeToejfdF0-5zj4CI-G'
    'kOja83q2a1Nm9MDt1QnE12Jz-rZhnJ_mTbDedo8tIli3nFGZ08cnn3J08hwWV7mSS6wddDPtkulpLJut'
    'wOzuOuQuBDt6lXq9vN6l4NdlkYhB9qq75fS7pB18k3vRQEtQuwG_83-nMweTp5vpws3EZ-c77HNJzAMs'
    '3toRNczQbklrn7t-9G6oVD00zawC2lEax5N8ojsRlxKz7G-cmsrm-r2vdz7pQbxWPXrb20MaRwV9drbT'
    'dhVbve6dZX89TOlYUliDMWerLvpmUbhkUDoc9PsfdqQF65cXrkxmwEYzZ5YhJdzbu4yBX4tRvdfTl4sE'
    'sACJUhfTEUBTO4qXL1FzIwZXKoAPIq6cjl_osmB0e8CocJ2KG7IzE7nrxwPR66c-GZXjQHhpKI1TRyic'
    'lQUFLfAR_DUOYyDPWQw1Ib4RQ9SdYgkg3YQWdQle7FGaxB15OXroN9t4IVbTmDEQr9N1R76Ko9STBz-V'
    'yVcNqM35gEaAf6ZMG7roSbC9jnovEpSLsSB7gohtnpnfyII7fDFBMwHNjKn2AddvaRFB7hGlXgX3AUH7'
    'kVJ7sr9tCnVSd_fCLer3CSYYYGOfAoA5RTdOyW2RPYTRYwlLPcxxUhEwR-N7ba7rj6uhlwVOTZjSQs-x'
    'cwUOPTnv4cjyygbwCTZzYHop8AgYSnwP7kZa74YDliBxvN7atjxk5-InYK0DcAxE7l8NhpIGNngIRdUl'
    'xg1wfOhlfjI2Qa-oBlFgzY5hP6hQTAPl-Ce1FIpN6UErFX5WpF7xRH0jMMysgC-gYA2Gd4mgRI2FogYp'
    'kWHyzn1_YX9Xvr3cICpyPjdewEf3iM_edIfXr0fapAPoAilhEUeiMLFtC-e3ca3kSdd_EodZirT8BdA0'
    'YGAPjrHNwDA6Q-WCD2Ii2uF4fETn3yyGrsFIBHN8XOAMMIp9ilxQV2cAwBixDHLn7y2AnAyAB47ESOOH'
    'YSGMZOAf3YmeLFNVCiy3i3lJRZA4j6PUuoswYRBXyWpdAW7KmgBTMdTCsRA053S4GtAQC2FtRPQICtzc'
    'XYJmDCNgUjtrlK7AT8oXO9CntvibnPvrz89XJAh3YO7FDP5ib080nYhUISdKNYoTogn3Gm_XhIGhCvCw'
    '3klWFyxrVhE8LqSBL8-shUZcf8c0fvirySY8hGOQHFp_V7hUuJuV7Ju2G33WrmiyE3byQ2vtgfNzerdg'
    'FB5FMf44XPlPqilZdLYN7DXJdciVI7UOZ1rpWrjVZHiB3SQ44v7T354U81_9lXn_6QE9d9U9qfZqG8P_'
    'FiEXMoPMZ7zZkb3V3GRYP8WaCB_QzQHxIG6I8JU6s_E0y9_vywldiV-WlbHcVqwbx0PJK6QWzjathRt7'
    'C8LERXX7WzYBQS5tCJa4TWOtwiLjpa5RACwkZHNH8sZ3MGTB2AD38NUp-DNEemXkIArnmy9CgugxPc_1'
    'jdKWYHsnDUi3Is3H6u_PIyJg5A3Ae1-Z8BVwkQHwObOnjnxoHGwlGBzYePgeSUIkh9DtIc74rG-ASv2U'
    '_onIwfHg0Lkp-DNMc7o0HXnANCI34ENAxIfg7SHHaLfVZr5yVbbgz_YkTAsT9KBunHWuYMTZ57MwIiyV'
    'YEYGYjUt78Nnz_5Wrqm9v6U_XOcUHpeuezQhLiIgmC2J56k4il-q57PxCxaeT0bSsVBHE2be3NpM9mC3'
    '1ba29MW41nau0IgPgY2NTC7L4CGZwbjidxdg1Ifg7SHIV6X4OMLYx3_CyuhTOa_li1-8BSOOjvSOrOWA'
    'oPklYmeBYUup1PSICDUu53XkwkAGZJYU99hlm2QAMY5HJZyny11VgrLKu_wqjoZTzfTtoFlZl3Pcrvw8'
    'WcMRvx3hvxbRez2hrz27dvi0bYlL2pm1zeB7c26vmXVLxGghTt_Mf6l612hRUaOMIyR3vIn1_4AYE4jg'
    'YkPwdpjtwCFBlG73rCuDPxMZ4uJP3iU4M-LMvVfli4A1Ml52WY2vWUX2o_zLFLkLNASCKWPYmuGSGP2j'
    'uuQTG-vTlNobq3bxUJ5SuUpJ_ERS8AuZXAyYD5O9OcIa0E-PE00pNhZiX881a9Kt4nS-Xt1HCD70qlYo'
    'd7M0drUap9_FC4mG8TE-gWjH28RD4qTRS1ftOu7tdbW5089aHXzwNHew6MzgOfIE_DJcRsgPrdu1W7as'
    '5bo4ZUZcbpczAngHwO3oD5K5Mhi-5XoJUVlCguXK837201amGZYIdPeRgf4Zzuj4PUb3amUXi4LGcSqI'
    'ZMeJK6pZZpxCdlqo5JpsIRKh6w_lFaqU5OKlbJcRTVgP13nOV_TMe3vlpNH118zbc4EdBgYt_9xUI9hu'
    'dsUJ6wFdlkU6VJzlU-dFxBoWKflKmUk0yFKHo9cfPU57d5W20mg648zWCbeMy6bk9t1TFhoKuMCcO3mt'
    'urrdaMnXOI99VBbvxkkhk_kWQn5AiVpX7EU8pUKJNMhSIprfAYafV2iqFMSioMSbkKBznsCJypaJDBit'
    'VrbCUMHkx-Qk2NKYnDfIWUaJEIyjwFsFpfNDNeQcysMZ04nFbSrGtKLFRYqMpXct4F-_t2VetOX7EgKB'
    '8iUcjQeJ1ohksl2gndw4N_T-jjmKrTDMNZNZj295GQDTHhGA-DBP9cltm12L7sIx_7GBWQhBJS2lg-W9'
    'JgNpttFJ5cBtj97clzm5Dou2WITSZTSnamas75TEgnM5jAJfI0v3BkkmlVJNm2RigEyx0TKslWGJPSCn'
    '9E1cE8jioxqVQkptU-Jn3CfLUyMalWJJaqnbLGclkKTUxZWy4L41DqgUpOG5DJucp3yxXvlivdnVbh9E'
    'ExGbKVTxmQM3y2PcB9YNkekWTZnpiUZ9KCLmWeSYvJWSYtp4kJf1GJMgvMENui2mUm0ZxSJe1K08BsJc'
    'yp2UyjQjHTNiSTyqXsiP0Cc1XCXScOp5U0LfrEBMuYUCrxlreK39Xut9r17oybBSzHPa92a9ecTjQrTi'
    'XaxRwUnW2VMcFUl1ONxoQRHuuv8WxLqtOJybEhEtP9O3JoP0JdlCEaOfRBx9nuZJtpgNegGZm0uQRlIl'
    'kBCh2eIF3J9qeULbvbk2xvuUK-qdqdVrNgu_Qe5hEZdXRPLECvv4_M02GqrFrIXLSb-l2rMW8rCNwDj8'
    'ZSxVq3-dvuerO-Wu8-vHxtq9Op15pzF-GgQlckK6Xm_6m1UZ-7edHkGQj4Xdc1WUMydjIeTkHn6vrW_V'
    'onr7f1HqMBavhwT3x6jhG50ujW5j4ksP528Gp8jimp3am11-a_JuAVCjX_BwoNmZROWN8RP1jKB2jMYq'
    '0bkN07Qlo2u183at1aXhHtPWZ2hzmnR-ca2HbV-BXGlY5-ZPDOgcUf1za25m4EiWYmJyju3DkHDr-9Xz'
    'W_n_cidywGiVmP6OTSayUkngO9r6pm1c1Ly9-LJuPNQchsygulsfFw7mcCWu_i6XqU22eYdA7krtyf-y'
    '5CnuN8e-jqerW6Xs39uHqCwoCBIjHF1VFrzp9An5J5zLkI9J9r32_NfQO_RItXtXlvfRSNy4XpeGpZvt'
    'luNVtbTWla7rH-czX3fdQHawP_cRB-OhL0arKrW_-q9StsYvFzsqM-sHn56ZjoI-Q3GIj3KVBAsGjO29'
    'g7KP5vbFTuKO9WjYcm5Ruasxvr9Q7qzSFrBBtgZ6HH72jwYxuv47sOdzfSDkCGwCSxhnSQoG0D_ICkvc'
    'uX_HRsU0-RBX2p-yuw-pX6LXp3Ke2d6lTAOBr-RlPFExTZjeIjnyTmyuI1Jp0h8-maXKGW3vAggZHUSh'
    'gdfIZ5IsrRNRsPcr_E5LMyMJE7duFcb7QezH0MS_3yYCVuEuNBNrEDuIKqbaDtW1WRmIiX1udhCPHSxK'
    'qRvK19gn-Zp7cFu-O_qHe7jerXODrPuNNHERttQD9SYMeL_aQGQhhj6GV0iLe1J8Vx9aOQA76W7ZZGjd'
    'UDXkE3woHH7z1HBuDwiB0k4OT5JT3G_7xVX507d5LZfPu4GU7SrdcPadEFBHFXoOgGrxmBxAq0_1QVNI'
    'TeA-vXOGpiDkbMRsSlzJ3QUwhW6EqYk1kcOG1DPEIc9YpAeNoEunKS9PB6t11f7YKu17z7OQDDVN3Y18'
    '0KWrpR35h7a8OLC2mXLi3gYeJOn4t8Pl1MBuBKswXOdOaMz4d3641G1e4stO7eDXwH2kq_Ei-m8TC_lD'
    '2GEmR_hXky42CgYsbCrht7s7JBgt_VVrtdrc7dXQ4ecizm3zHnJgsf4z7QGH1bNHR7_yGT08Y9D4wB0J'
    'Yi2qiNk0GdyQG9IMaj3izYe7X6vC8zU6b7MBhYRgzx7N0JBIsfbZ7g0daXQBRySs7ktSUJVxqtrbk7C9'
    'pfWcB9BvpO2_Fo2EYvEI89IOQQtDKjdhhAQIyP_RvUYhA2f2i0QrmZp_jS9Zyl4Yri1uful2eJPYugJa'
    'sj9MuR_BLpAWZwG5gg4ocemUnYdNexH-obvwZ7TfpO-HT-SC7zV8D-kz3nHoiC4vE3YvUCkzVlZWGpXm'
    '0171fN-vw1Con_gaM5AKyzEankNdDgkRh9FHSR46yBXf6fV_M_eMVlYgmfXEHW-ZyshXb4P8S1jYe9kO'
    'GlNcBfnz9qKE8-MWtvba3ercNjzPyXntsDk7-k5CegciQAOEdnuPBO83z1EdVxMtldSOnYjda96lfgIx'
    'z9PGH1w4AoUGTtHoc4vJDz1G4UYUdb0hQOZlkexOZfgZ85hyuzqYZi87f2CiaJJfP3YALnQcJELi4D5r'
    'QDINrUvUXfZ5ryzTYYFWYlM8wNZ9vAxlGfZmY53aJY1DHDrHK2aXDOrmXK1KVWIOeag7nbM3gjjqlGIm'
    '-DYcnIQ8hN4T-15toUl5zaaedHM3x2mlQldJ0hkzXJ_7pVdYBSl9KnuAr9Yv5vI5HwfltrBnIpJB4H9E'
    'QQXywO5AeIZ46878gnUix7QucEeFT0oFNF8pN5hXpZqlzgCXX2Ey2gAp2oF7khmz9jgB5TpKCB9mg_83'
    'bg_rNZFZy7HrJOUp8czcUKZcKxuiOgFrMbRzeMpAP2PGnxy9r31R-611s31mv5rscph6yX-Ax3XA7JOc'
    'd7Pf8D3M3gTxQSrSyvJI1dudeuooLQH5rNSmkHqXZnDPs5hl5izxf8L6kDQVh7EUfvUqaAkmfgus3lUv'
    'uAV2V89GCyCkLGl1ob6EDnsbvGTytKKE9B5ChugiIx0Z7VWQ5YaGvKGm-TOlFp2OsEFFqqg5wSvE66BA'
    'ZTVgkE4Sq7QeqjRM5o62J6MJhJUybkL9gNhkkrFUIrmQwo2tXIxChk9Z92SgA0yIAmL9QDEiVIdWCZcI'
    'Y-oLYzIDAL02C2UcbL2kB3hRKfeW2RREcM5ZvINrvN6CZ9RUhQVFar6GkTWfE8GS_kf_3lfDA-VUk5_U'
    '_Yd2eRXMOfJdlc7wm-TqgLP9ppHPI5cSoPHLIwC4AhXq3swRJlDf6kSY4ewexnTqOkYe8uNzwdwIQMwD'
    '4Mpl0C3yiMcNgMKFr7-OqJOCYE9LNqtVFv_lrSyC9qqTcgELctsbBgF94P2Y4Sl-UAJQkDAMs3gKEDKJ'
    'Ivvr9qPVi-aNoIxBmvztDAC6gSlPb24-0qGH_Chk9Gn5Qv36Apn3iggjx7QvgbyM8huXnGbfe8zEPAUx'
    'nLDPlolY6pCYeBFjmp49zN4vwls2d4E_W-5BQvgEmTn5AkyoQ8o4ysg86sOwAOAaOX1r_KiT3O-AEVHW'
    'FakOs8JbFv1CThkFTUUE3tcVrlIR5YqPGGZqDZdtH5J5pBDgvgbAn2mSeGOA6XYFHhr9VM-jmbYsLJHH'
    'MF0oEfeiVOXQ3WgWlYDR8sNECU8XFns1qt362vLnRdDofqAl4-kuXyRdFrGTshdUzEjIWZ9aU6C5-7d_'
    'MIWZemCqNXeHxZfroAZlWRZMdlckdHpbO3Yd4Fa34zRt-cf53dKH1sZjbbs9J2g710jp0snWImdZAncF'
    'n91K0THv0k4pl9cb65WK-ubbmxKU1RMjvXG1W1-atdt7F2ht7EoxY8zp9cInbi9NKtWf3Sl2HybHlGb0'
    'fyvBQJMm4DSQURnAHJ5yeZJ58-ZLbKDF-3tflXXdkd16BesCOOE5BcDA5QgEQaxKOUoSd6QReQ0_Rk4N'
    'gRlkWnGxxIvo9ypTAJNoVJYK2atARupNPUBBL44SHK0VEL222kl_oiwRmA7UKDwkLaY8PO04UQ3x_fFM'
    'CCpcYk36cMFZA-ZYdTkhIDT8PfBSBpkb1MErIiFEw881diam9Gou2bf8QYZYFJbnHp2tEdPmPDyHSWmY'
    'nIgo1B-Ha4bFmQ8VxAwMkvEribb-Yx0W9AbgYlnmifLAH0FGOT9HVbMT7D0LDXSQa7h5CPe2QdJ4B-8i'
    '8axLer2ZxUQmdw6gIDl1kRtPvxmDoUels8lfj6C3f3SPPKo0aTMQzXQgVU5W_Sg_PFhSVxQjh61lN-eX'
    'vqaOoRVV0mcdbFBZQKDi7eElzVd4tAFL9bJDZqobte6y6s1poLd6qFu61Go_WgWlu483ChRkormAvL_a'
    '7VXnC92dhsVB-5SuRKgUF4Rs488LzAOf5uEbJZiuuVXHaIPnxnT4ev7_yLvEup8yxPvKcQ7gz1KlCuIt'
    'EoEoypxCIhFFkiUSAQFlwq8bPoVrLxk01vtpHZ6LktJGbmm9pWZ97yiJukLgGrNCzw61UXfTih27Yd0F'
    '-WrBpigUwQlN-ELz8ufLFz1b1WB49KGErgCYtND9NrQUjSYHD9Avt_mAXmZPUj7uKwmJBcPVh0OTLt6A'
    'S9PgmBvloi7l99jrKQLztGOmmowZnBoIT8cZ5PTFoLqsamq9lEpQ_FoxePrQgi9VgPtkPpV2VGOkJjJV'
    'hYvZ4E-6y3BRLgOW-LVahTosGcpVwMwaV4GITp1xxy4J7u6kbt4ZWttfrczTL26daAiLN8D1hMQQu1Jl'
    'eYSQPU6F6vau3VeRuDBrPfetN6b7xWrbbuNev_FmaD49rJ2SAh3R45ulBAd8X6WYPwPcSOwgndfzNmsI'
    'DRr2HX9EZOBTSeIDpkgBn-zfnHh0NFwX1FI0mZmBP6SEz21UEnxtHsxtLj5hwCXkLfPc6BgRGewFF-cS'
    'rG1YF-Nm82HMTulMdfI3-jmn9EwVMcSU2dxyPejVZQI5L6en2LObL5uZuGpeedT5wWPBIdI3OUEw_Rgx'
    'RP6qFKsmOvx-tatSldSv1VpGQ4QnyuDIsJ6Y0zJIZne4W259Wl7mWqTH99_koLMnKKmlrSqcypIISnGm'
    'FGF1-KcoyzFvwZcRO-HhfLj7LYucUCYsdje3SMgg8cxSwkxXezrEmJO8g_QpDjOMuzD-FdCt1v5I4vOG'
    '1_X_sVzDNQizrZyinY9fuZApb6zEXTJdgtxeuNLyDqOd2CraqlvXLbh5RCbrsmY4JULCK1eZn31Gvxpf'
    'lO5eOxBykOs49StV1940IgriXJYb7du4t6l_0ri_1XUHnUeKtF-9FoULPMR7IpSNoa7sfkFyRuO5lsZ7'
    'wppQ63SHidAksJhKUFos5wHPib2uzwoqevt5KxvtJ5a04GJQ_eA69aVlrTJpxwExUReTscuyd5SniOWq'
    'h1_QQVDtBsIr9ro0xuv0CXrlWdzdbcWc2bnhZfIr4ZdY3D3T2C9G7mhyLURuHr3MQElh7hfSIzwjt42v'
    'QxXGqW0ZiSuF1M1mN1tbUVrSFY8ORdJb0pgI1YKFfirFjCT29SJk2Qpbaz7ViwKpHJneRcNCxr7Ve7jo'
    'Gkc2vNW8kjp7Ir3kngVqkBQLMVANWpJIBEZ8R0iF76PLpvn9ZWv5cREud0VgeORi_5fa1OhCD0mCRzpR'
    'JIAP6PsXTOac7r-tzt8_BOcWZvwGKB-U_654O3_UtZJt-BDxYcFgutuwtei3uh21roAEeysOmyua4udF'
    'oblUtudqugrfnBQne96lSxUGe9tdVYA2F3u2pU92vN7oI7rhfutLrrCzWQhSy4bAv362tVy1QlWMG-5B'
    'Yk-E0WKCmASFD8ZGQzMyApmVIMaa7eYaFeyfTkBTSs6DUj0-x6hlOSNb-XZqDA5IrTJuUAObheXyABqI'
    'HKKC0IoOhFos7-Gg4CyWFmuh7BNmfK0ma6lSoZFRSMFPhphvH9_4bj5mtDxCVzM3lbrlzQRAiEjpw2yz'
    'T6mnEQyeIpLptozt3so49MeJpN0BLtcOHSTeukN1lw5saaAejD-lmmMQF-k4AiOxM8GP1Qj6wM3mfV1r'
    'UXXDiCwc_HgWBzULpuolt8vVnwB-XFpxi8c_YtnzSVWdwKhJwuHHS7Jirn2D_yP_HdYsxO1X-3mNSj2g'
    '2qtvUOnB6W-0ANN9-J2Zo2su-VFSOLCx-Al1SXOXBuAvJhtjVwlAH6cNvZaLqZhHEIu50Ns5uvyltviX'
    'n-on63-8e8KpSX7CgrlfMacfmCPfnhI2f7z5H69FGzw6c3UQulfbTsUN6bqMUiuS5CrNzPWg9K6zlLH_'
    'E800T2XF0fyYj34gODVYvPkfrEuNVppHsFou5rEIWwzsW4R2y4E4SN_IAY1NsKG_kJtkHbCTYahNgY0I'
    'hMVDPY6An5snW_urJZEkLxy8EBen7Qj4HnsrFNivdSEPQ_BY4yQBiLDBDGI1MrjEmmXhiXXCWZsblaa6'
    '5Go7KoHSWU5wNomAFFUwkF1BcVr_kvdGDOkT3yykoRNAtMqs6ytSrBmr1fqzbcOvEjgeweSaJPLGgoAP'
    'Yk3kW28FjmYUAml1BBi6D0yKU5IvyuuWtJfL9QGQyQ8kfygiOqn_AMP5yq1IYjPOGcc7q9Wh84C9QUb4'
    '-ETxmQrveU_FKgXygNQ0WlxLreDdJWlJjfXIq9B3rBB0wvzmdPziSGrMNDBTUk6JSgn7vLy0EURbwYOa'
    'oy2g8CbDUYYlLC1sUEdUscId-6Z_UlkgR9fdOJcqFlE0WLSi9bcr02oVTGysECz-ROxEarlpcY0sXDql'
    'wZZh7VGmwuWl4aGGqjR2B1PeV7Ecg2JYi19IyiP6vu5XJKgkMsQZKb5jVbCfoHUG8Fgdy8kkAKViFeCo'
    'KQprVRdddLkhoySgXZ_MfjjD-XmDqJpplknvsxilH3BBQmOoVS3rRunz-XgmVUi5h3kriR-f1W1clbp-'
    'PrQt8-2AWgNuVKc0qwvYE4ellt3Cn4CyktGzuPGphugt-12vdaUx7D9sJ1Q2HGGiWlIUG7OK3v4IGmk2'
    'BDXlI-csuNAsZQ-JVH5sqLbzd9evpBT_-BkMpXTLacIzCqR_HHPoUmVDliQA3pgh-JcGBw8aHXNojixH'
    'yjpzxR2xKQa5yV2mciQM9Itsf8qpPt9ShFgh6oGJjLPTUxiz4pCOQT8ghnhxJyTx0FScLp2lu-p8eAju'
    'r4T-R7CZOQUXTKyp2Kp5M_QylAxSnrBphMGhfRsc_ra8U9oVW3SO4IgYcl-Yl8oay0ahQuukwYo4FydD'
    'HKDE1_ganIaPKzfqA_wGe-PT105ArNtH-tKhy2_OiXvCfhGxm61EoEUYdIFrbN2YYkPZxWSX0UFzBpxJ'
    'uFK5U2mU_04cZ6u-BWOWEe86zjEa51w6EqkDeYLQJ3FROHC3Voz5VMzoCOlC5nivp8Rn3qoNzB6Isgv0'
    'lOOExjmJmnxLAbfsNWD2OT4chRvcst-adwy8qgEBV25AhGdZvxmwUtATwbU8zTA4GNAMjTar0q-KjlhU'
    'mYiY0YD73QlFaWZNx0wfRd8BWyxacpD5vHJtUs4jlMwOi8MTEWJ_B46LWAJomPwmsV2LHeaNmrH8GRpV'
    'uh05KUkHueskzIPh4DhPcg1rGjORnvK1RPyfkTegrUy1sBZVf2mY4EMwT97sNuOjKgvK50q9mtN7fy1L'
    'lw29I1_Km2urW1cbVRFWJ2cCUZU5SYIZdk8CzWL--WVi1jeK6kpLHflfxVBvbz2MpVvJZMknBaKsGPbJ'
    'mkcwkmJSYonDQAEFBa0CgBwe3b5mJhZQImgWUKRqFlrpKBYvI0tgbgJckGWwPyEuUMtgk4Spcz2OYqGS'
    'ieWGN7mo6tBY0SEGBrczG2CZiwTcEjCqU-dWz1QkJ8ExD6SU2AowwQsE5zMt6ZBMI8l4C456vKCEqvb9'
    '1ZLdpteF844rkoKQ0mcoXLJRu_5B9OybZlO0lMGmgVn9C8cc3OdAzbtfr0_tkHGl3DH5qr7apWrIGuK1'
    'bVbHfMEWwTZyVEA9NH1oKW8XTcPqtm4UYvxEmV9Pj7fOZLoq0jviTusrnxc-PugdUTTDfSRbBoLhW14v'
    'vZKUmnyOeMHpdRkkAemmxu8pST5KU7XSa_BZzraBB49uSHJ1v-c6Q-PbkKn55QhdKeRIXynjjFIuqtKv'
    'SnJz_orWoksFBD9cx80kvVSGGhy9Mr1SiLhZ2engXEJ0yJUTKrzzKg-JypsUvri8-aBSzVsujpT3pp3N'
    'P46XX0LAHQa-OexczUQ6-NewWs7KrsWUB8dVOzaRfzswwovreZmU3qi29thbELoybGS4yUGCM1Ompc1I'
    'iYViy_3GpMeXtGG8C-1SrCt3t8gXprNs_X-JoUwXoWgKvDgEYJCFeIyeXfpC2YV0oCptWSqUSuY-7oa4'
    '-r-CRMJWBkAISlzBFwVECPoQYyfrZ4jtV4UO-urt_409X1WrNZ8viZtZOxiV4VxxyG9Y1pp_wrNqcsXH'
    'QcUvfiOfru8TiSe0vmNRiO0wx4Rt99mCgTESGtqOD4LLZKBV6mCamVCzhAvVHVvm5fbd29W-UZDX2IG-'
    'CN1qetun3EvdFyG_p70bVE60k4KqYIJ2U3xdO8EE-J_vZFdbf7O6z6V9La_nPV8ZDOw87lzoOq5l2UhW'
    'dYPJWRAp7Fswp2-ZA-KUjZMGoioQmKb-Hb2kM3kg6xK436anX5q1qIphGMP4QTWAP6c2sLpCfBnhEdEE'
    'bbw_FxeBS-ydqo4MlkmNTzZXWRVifahoNcmibzYihOPpFQZugOvTwiItrLiF104NoIIkYvXJ54X7xRyg'
    'wmOhTbmgNWBbeJ0LGgyYH-QyK7zS9LKxzBQhvsjEXgq7BNwg_CwX8esaxHRnAYa1ew6NiZXgp9lLWxdq'
    'zKRGpfusiUrWT8ZXJ9Od_S5jQbSokHvVmFz2UKQTf5CzqeRszxYbmH0BfBzmbyHKZgOY7fMdwSfDW4Us'
    'lIggwYn1tHWmzLLl8csqBnSKKlHicBI3WzJ7e987D87rGnZvndEiSOXwkrxKS-GZTgHNQAd8ASv-zAGS'
    'nCbcPClcFhQMYIQLxcYY7lFWam8ckEZ-YISpGV0xB3GoYODJuVdg1ePbczIO2txwOP8mCOY1dIxMAeyr'
    'PRL0mzIG1GnuUIwXLYck4dI4p-dtBm7TqNrJylCxlYQA-bU6jxmh0gRbB4cyJ52QC_hi-LibRik2QILl'
    'wCF9oDEvsmn0ALYieHSEgEF6Y28QjbG4rYfnFgwemrMtNGMHmFfoY-bvIJOQxP6PEnxY5JGgRFQuWF4z'
    'QDCpzTkvQESFaWtj1c6I8S5MlFCT5npajg4nWEKTNMIYl6N7LJO-jHOVNlpMB9_4zh0y44Lqe6F6Nl-b'
    '8Pa83V9Vb7drd1-w6km-SbKS9yMazs3EF7K2n2t9FWMdNoFdx4JE2K0zu0mFaPvQJXxthSTM5Vs4SkFD'
    'LLTbsczBxzZW5imUu6yOWAj8foPS2nNJsXH8zcZVMc0AlrII9m5r7i0TzCaFGBPbHk1ZDgAtUsUtoMZU'
    '42j9kXRPmmsBOP8XgH6hG6Sj4CWDPC5_slHsiP1Ycckz5VxZ_rVaPREpUiL7WPQ7YvNRUyScqSI6gOuL'
    'PvBL3yIXIncWrY8znwa5xq0vBc5ev382waDMoT4dnR8TG4t5-oUK6WnY1Jx5FLpLix4XMPP0_jhB2J5e'
    'XWXYyTS2MYeduwDjEmjiolFlnCViIgFPaRKDKxRlIQ80c0POr8yp5bwi3mArr3eyOq1pAMq4rtq1Jc3T'
    '5wnSgclgzcU-gYbs9cjgEKmIOquAGjfbfAew-fvIWW4J4NGIKAsAQdkiIU7Ta-nysl7Md4Rz-1AmbFzA'
    'uLUPJ3io_mk588ULSwK0ODqfDhwA2SS5kQk87CwwqeyDAGfh0cS0qnDzUzzHLdgwfRYwOYqKiSAuFchI'
    'xJ4ngwFxdiEvziZvAdh1C1K578PoUbFDC7ezKubl8GnxWXzIiKuKiJ-gfIPQxD8OPjsQl1cEYhXeipep'
    'sDXQihhJAqvEPopIxYYlFqB1VfvYNrFNCz0GNzqPSEfQ1WLuT1DE5ZjJcKxxxC16tut968N-_gSmMT4i'
    'IgIKgWnQGSV8aZfkSyNUj2sx18RIfNpbzbiF33I0ZYP3Ls4XBMyojHmaRMeAM3FJ82tqpuq9XNW_vf8a'
    'mSvLzADXiYgPylyIy1Wxrz9kDRgTrjMQxH75H6RE1zqVIOO076E5kaamqS-rqNIB8HXffyy61OffUtPL'
    'xsQH7xDYFeHOrH-nbqgaMUOFLxWghIWvYZcHJp44SXBXDIv2iYXexmrTHF_YobixvtWtQIO8dYdCG_xC'
    'Lq2T6XYIzOzVq6w7CiI-7IEZI1tomlAwQSTjnr3hcTTk3uyTTLeVwLj3BpvEK-WUfNoQR1VZyQ85IjA3'
    'K8qAI9Hiu3zG834lfa3XrnbTym1bCAANypqcvVvXZrSzlX_1_j_zP-nxLQ947PNPAUlSRPkCioySN3Si'
    'jjKYDt7Oy5IToNGokIIjONM6CYebBXjDZVKZHhhMNtzcyAv7X4gWPsklAmCWRFC7SPQqKkVFy5ybCJzQ'
    'vSL7NEkcX_KQV63N5txTTubG28zYKB_HqqYYT2xH0GwQd0z9PYko_VHdwcRsi2jSG7H5Es8F07A75EGm'
    '-3ATa5iEV0AHeq43SdclohCS6WpWKQlk8i96LQezMoTDFyNfq0QhJQlVIxSBNJuRN56rBjhiUC9JjhRS'
    'Vj0t_AnvT0hAofz6HGJXtS9Owxupyf5S_q30c5d0Zg47bOo_DqBlo_Pc_xh5DxJPpfvlSq5dIC5b6UG5'
    'wlH78dnaiR8II_SZyyvICXNhQlBMLQcFh71uSYpKTxEz3iCzYQZSI_AR9Jus-PywwL1FtlN0OrU4d8v6'
    'uqtTvyINYj6VulVzkhXQjxA3Nj4N-AC2ELs8EHQ2g6GQMxCTroAQIRBryMI5a_eolQdSkvUut0Cwpl-y'
    'hiFoHun9HSzVXzbas9b_-4uCr3xj4GWtLk1-21-cfFJl9TSpiCb0wgVBgrewF2yxe9ycbJLt-rvtoqGu'
    'e9x0ABp_cjahOIOw8ChZwHXze3kQU8ssBjKbpBxYP8LMM5IrdMUIvNs4S9aclCoTWb_EKol5FE1mt0xG'
    'tur5SECmKFJFsxuaIh2zNVrU0IlSYJL1L1F--yf_6R8azLe_abiPgEYARpkYDG8Gqr0cqvRrd8vDQ5c8'
    '6UYo2xTUrYOj7Q-s_ikpBUd71aLblgxPV6hDf0gzwR-LLe3CrRr5PoYiJX9PPWVqHv-KqdJZb1gpwm6U'
    '4KJ0xTOKARSSy6asgLIv5c1Qqa16g2ktvR6dRV9wuqWCQExh2ozroRvaoB242nVFFOcnW91r439-UOtv'
    'JHaBu6S4JccTXAQIfohxY5TyUFDgnKOonoeAKId3nTJTCH2crrKaPis2ZNaejVPY1tAiMwaePTNuymZt'
    'FU_CU2wUdK2GDjfuokG9ZntbFZtWvdrXbBEHQHTUuJZFCsv5yvY1iVWxv1tXq3YCUbg9tPOeuu1AvLlY'
    'SA8OzwOEsPQHb1p1Zjq7DXSBkVTqkwLj0v40LONR2WqUqIdHLTjRGunyghRHWMi-RkQMkLfWaZkDYIS_'
    'pBrTALrN21W6Jp1zcdZ1p2MDHy75P5Yf-21il7VDixotUTMqSY_CVXFYYHLLgbwvV_LJTrgcn0GNGmlw'
    'LDLDvxxVajUbuTX2PvQTMomPMzVBA71u-sNuk0Pu6bGSwdDu_BpuF2y9KZrUZxy77POPjoA-Vt_k3V2i'
    'xM8fv5ScUdepAAplOML2rNe1u1-R8iQenRAw7cLveaQ-GF9CWJmLIbobpXb-W10d8PLT1MeA-Uz4p7Xj'
    'KkjVjeCLHqhL2tGBCZWvETdel4A4fO7YeXr211OvXar3B7U0-zTLXglvuGXdu8XIBry_iAjyMpsI65BW'
    'jyY3kNcy_mPUl4xaYYHQX2lZv_p9ZGfd4L5BAdVL2UOgIkKM2EUAmIXF3fuu-OgHmPwxM0cuuz65hS41'
    'ca3drch6HvI87gbniUgCc_4ZN2cWZqd2rttfkvDVLGQG8zHgUFHsY5o5jcj5DunIq8pJgCygTgMmVKH2'
    '7UugXm_33OI88lzxi-dtX4FUbviBl3KQuMwKko_XFtY6vgB_E9hgP4sRM4KllFotT6b-9Xze_nvcgnv9'
    'DVy6seFNr-qmpW3bm_ypNJ8qxd1Nh4OPcQWEd4DJ-QP5NpdOX-3Jc_qf252Z7eb_DesV4wSX4fckpaaT'
    'OH_atac_4E9ZTvLrMo559r32_NfZu9RKkyb7GoYaN1ZD65Gf3gc8KV6C3CJP39N1W702rmE7-0IhcPf9'
    'ipGnfzad-0Oh13Ia_fr6ZWfWO9Zo2AQkq9k0_5c2tLj3ZA8_3Hea2-UW_ebkazIG9BFj8FJ-HdV9OHUI'
    '0VWlakkiM_noZpzHbDde9t-yEU0ozCF-iMSiT3I5L7EpXtkC1FCGbiLV4939_4A0WWOlK11Y5AFtgAdN'
    'TnX4QKagAMLWCUAGbGrUZt59Nw7U3AIx1nKILfZMDgezEHfpatBAQqBrus4WIW7w66KWfATTL0sk_emd'
    '351515M2c6ILqdLzNb5-sTEa359Yp0URKA1UDoC5e7AsRK8iiVlQ-dsv3zTC5Vp9dH1nBh2tAkNPut9S'
    'nhGj2KH4FQDr2YMn_svHU7I6UrO1aBM6XHarzP-_qmauN5YpBF87fNbn3-siUK9WFMd_91q-p0663MIf'
    '7PnPLt_MOUkh9Yqex8hG_GGF47niWsdHw47q_oYfKofR5d9FGVuv7h-GRhHD1DCiuFoM2qVLCfxDlJB6'
    'F6-wlB-5MoBpDS40z9v8JtzVjABEDBN29I1T53UcXJVEXWYOLRLTM1X27NPdwWCdNZQuzxW6LpJr-qKg'
    'NZzrLta49M5hy0iPEXrSiEZ6XrV1Id_u3aVjWILSes4Wcay2tUb7TcCdl8GzXU9z8iu9BiRwBIwsl27x'
    'jwWMg6eXfBO_a2PX-i8v9ROWlUTED9XRG2Mhw3f6raD6fp0rW-rTWnsRXvH5cSd9J4oLUGGSjsoiM4mM'
    'pkJ82bedtxpVDqQCgHuSF_QtIhnaCd1zNogFSRPV8CMVTJ3ktICjzVGqDSCX6YM0YwVTDWmIdJ9CGF9y'
    'bPLho24bNWsyvnDk1B9LTaqf793GPDw7PTq2jaNBSTh1TQpHvQa_EAicA3puDkKUyypy27VI-bvxcGFA'
    'oxQFYM5lqiWjSpdwebBPhW1Chd3yxF7yaLH5ylbW2pIrxKJ9U1qir_4t6BlMB_oBO2s9Tr5CFucuPmE4'
    'HoBiHxoz7KV3MmEwKQNYrS_ElejEIWsSiwccHW9JmpUw2J8uv8HppDFIM766gTXUHvape5pfyT7aSEns'
    'EqavOw37-n0hjLKywZ4EgDaG4UaB9tuCMg4_aHrMQzXnbSBD_5GfCZ0JOVRqX5YF3bxm-yGgwVNHVeZE'
    'QFWTXI5xPOCiUmJpKqca1XSNCWpTZPaa0fsUr8UUaVh_H4yRih-Ka8lFIssvWidsUBu2JMd_SUJDTz1-'
    'xHBOdeqx0GJZ_e3g38YYZ65Jy7j8sxd93mr90v9HPI9nChgX22GE58IRK41IK7JxajKegDcsUHkj1gC0'
    'o1XCdop21yFPX-Wp9VEJS0oJGDdewJARP7CJfxnF9kKsXgdQWZtZ4VOf-cEMMP2HVzTH61bVIIC1jyMl'
    'Zy1nyC9ufPkVFZSeZzupkkGeJMtpk3OLVDAC68ShvDeDo45IfJn7RXnuDyXFKCHXZyPbTtYazikkqh8W'
    'rmPZRbEuQdl4t9oCuwo25rlpXkGrTOopOaVZe-qGoFL-LeOXum6SQBESVT7UwMlRutf2qWCIisSY910V'
    'Q0o0od1qVmj_aTGRhnYpUmes4pE0AOY4pcy9vHwswRTQ52oUMDZKPb5tzKCcqR8Umchg1WHShGnsgdsL'
    'mVLSNUxEFMFlurlZqgOCaq4PCKWVAfDcA4w7VgvXoKDv1Q8VEfG6jsYYHjs2zTujf1Trcw78gSi0Vju8'
    'T-wXSHUqB2MJbDoWiLIi1KZDPWPHzWmXy92wpXlAtKSTJpEgEJO2ek8FKZMof3-8hpC3Hr0gRLYqfH0M'
    'tcu2KEDbv7VIItRSrmT9JSKiEpxd6jMvGiTZI96DOtjcpteZqSiU-XSdIlzxO_MVMqDPG2KlMa-XDBSX'
    'h-MoScZEJYZJIUbScXlZbrF8BcbhmXJYLiDRGXq7kU0O6RI5P3Hs-uUtPIdXZr_rHkHT0YMLsbSCaGdi'
    'YpZQJzJcEECtBC0ReyTuRBaD_m7Fm6s7wgrOrGozxmuTP1m2LYKjYizoUrR17u2A5oJrevJGXEfHSRTC'
    'k2XVRJcnXIZFFxkkVfIw0xyuMbE3IYj8m5X-Kp31alBvjTrcIAc9RGy54cYYUZl__0UmT4Fn8D7AuZmG'
    '4ePcG8tTMQ8v6Sv74c5oBDdgA8dbH9vlHiT0lxWbEGwHIepxxQ4eI08rY3hp6ysVgagoiT8vMcjGKMWG'
    'YW0DpXywStvdGaYtYUDIhe5_mQT9kmqsCxm9K5KbhWrVb10v0_XkyTrYque30YpgStRqsUj2oPb-1HCQ'
    'MUtkApg6bBzTvFJvj59Fz1XKs3S9TcB5bps_TNoOqTR6koOCYcpeusnMj0L58cfGPk0QnJGh3V19-1C4'
    'TndSqXOkTbq5y4KgtG1HWCXg9FgQER2IEdDDpVEjCdsAMVeC986hZb9cKLwX6K_z6PKfoWyPGWSQkSVl'
    'gw7n0LPFd0AInBE_K5n4Aw9E8CHGWAk2fR_jh2IASXCp8-tFQEjAxA1hT7zDEBdnRNGjAyAFFTGCaoR3'
    '5QRBFZh_ykGhb1LakqC0gnO8o7P06QBmkqLC7J6Vm0kzLnuVh2E4oLkMs_2S4npFWda9WE-nqi6r78GI'
    'kPOZcW0WOa0ywYI6tkE0aFBD3XtpUURPOe1p4Cdc1eioh1hg-qLX6O1GdmFX1bsmUhP9laiAQEy4D0p5'
    'ojUoGdHRSJq8BgSKB6ca_q3n5Q1brrVfv2KgaSKClgyFgG0Ssgl_0NZRLaGW5ZbTi2Dpz7izYur6pgFZ'
    '1GK1oAwF9ApdVcq4Pyye1a5_sq8RPcfbhZOcS2Ot3WxuXNdrV6GyAmU6e11V6tbFH4u_CNK3IjLYFp12'
    'r15mftlnSIVf2wWa12q7XbGnuFTaYqxs-UzmTsdNv15r08-gsXfsvlr9riD4Ru1CxcIIzDWq1bdesb1e'
    '12CQ-RqYQNKGSJtLXaw9ubtWjAovDwI-Azler8rPbwG538QKqV-b-NWncVzIFvU-Wt-1W7Udt8iwG4V7'
    'XeevRRwa134Y_N75utB03-uu6KrHbVh8zAdbVbLTUuF645wOJMndIL1x06jepb2grfttprHVOoqGt88_'
    'LFhQtftb6Egfq81lxbbdfuupXj1UeuCYRuXrhRNRoYXEMvsLgDFi7HQBic-2Ka2-8lWhrXqk6r4SZmhU'
    'dOfPMsR8jloNfiWwzEEWbRkSu7w79bjCPx4f1aYwsytV3hENwNL7ZG1cHIC0681bk8nu8qP9ueZJP3bQ'
    'OMHs9Ngo7S4BPdDbdTuX_8N89rqAGocI6KphQY_uMmteSZVRNoGNZ0IfkOfkCeY4fkR96rg_4Mz5vxeR'
    'KVOYKmFMnRw0MMXpPB5hUG050qCx8vrHe7m51PfvObTvfyQ4dn9cPH3aq2cbm99ZvP_nDli69___Hf_8'
    'N_-YcP_mH1P__Xtf9297_9p7t__1-ru3_7t3939x9W_66qeS8j-smWcb_82x-67ZroCHfw81ZXdet8Di'
    'JwFeTcPsjlEV2e0lPjsVHdYg08NLKTStD4-oQOIbajiQeaAoI8iCQD5PqfTAWhG7CFvqzlNYVuFgvHGD'
    'cRzG6uQ5nBrUw1k20OQUMxuU7IxRl-uiv8yXi4nCsVg_pMnmXTY41vYnqMBNQvoxZLAg_-UoxXAX3RY9'
    'Tnz7V3mqlKDPaU-Ekm8gfOIDylkEe309DWPhms9725AGwj9npDi-WWzfqapFW2HASgyZc0TtXZwgH39n'
    'rrQcHQCtNgX0x9ybGvlN6DHngDf02f6nETAOoF0-eIiuRDn--UL6fbieTF7yifk9_xZQMiVSgj-U-0wI'
    'mgkooT_C1NacjUnFQTR0MNUDIABiUIeoPy32fkmozcBeInVXDs_WIux5PswvWqmjld-hWT0bMvmBSVhw'
    'Kl78puoPW96hle4I-SuWFwLI7eSkX18EKQbeIMnK_rwXuFW-Yw0cr1UvIE-cJj8AVQ2LEjFI_jbmfqOs'
    'f7ED1U-bnEGxHoq2DCCuArLB-GK2HY8MQ7TlaWN0ZcQRsA1N4lAfku4A2lFRirvckN7uHaGK3csntRNR'
    'ZRQkVp_owCKLlu7ChFcjFYicTA5mKzDsAH7RbTrOTszH_sKbPKRX85JluUHi4c_M_kL_QFiv5nImjYAc'
    'cqYBq8iZ4rHW8U1Cv3cQHgksMtidsr0F8a1gMCn1LktDNqeZuAwI_gAsWvQ8ri-kfb1OeUtPTAzzP9gB'
    '3if2AdGJXNHwp4rFMG8kRyGEpL3_v0uHdAbZMDYT889OxHOB7QI1ksxy4YEdkJuYQa-Kh2qELlCRAr3o'
    'VR864b-aVuSHUYWwRykLqP25Rcysb3Tb9ReZ3S8FKcE1y-T7wT9MzNIjDMX1Zr9XD-fNVqMq8mnHj5xE'
    '_JHX_4bLRaa249dDoiz5f1zqqFXV9db7UaMof43Vmrt_FW4o1WxJWJcJt1FrJzcJpT4ULck8sXtACF92'
    'aYndTdt_Xv7HZXj504g4U6RkBDd9ID9lURCgony0jHo_trWsGw5s5oGvEcsCYhQoskHg85ICkWaYpMz9'
    'SPvXvbIVslkJ4cc74yccgfuBy12co4eAv3RAxp2nhfG0nQzjnCk2CHAsNhvFtaguEDDiMU_PEoiI-Qh4'
    'T1OKp9VEfgs8QA9S0ary4YDAmPox_h0Yw6lSaIhx3IgqwN6tmM9Nx7ZnPIFMoyQQYkhtUER8XavPp_8E'
    'McXOXLNybRnyGuoJPA3zANpZ8x3a5W8quligewj9oXADLSZuLee4C9o2GkD1jV_ElxDODjF-gFfiqH73'
    'votXlgDb8_cHf8SeKqLl5Znn-3-BFkYQKNwRyZ2O6RyyEfWCnkh4BGQAkvm8njqnsZ13g0ddxCiqDnlV'
    'EKi5eTN5Od7xb18CDQllUbHXqKxxBcYIBJCfeyPNKl3HajHcdL2Bk-3j2m25G4nlm84oKYPPtIkbWPvH'
    'I-MbnKZA02haeeaD1P5rU6GBaiRY_Xbp7owBu4rj0hbQKdy1tmP_HzGArqcXgmq4S-7Yd-ZdqESyTe9H'
    'HYYHEXcPMn-om1_BJ5gMYB8Sxmvilz74fImQC9pbIRIw1uXm1nb-mldEJW6jhkR7gycsMimz3kV_RHen'
    'WoHKfJBNwkAQlHFeKpnOzeSqqgCV5IVLcwxx5YOPFhKVZEv5wH7yEnulfJCb7QuntXVmFlL4blmMoC4C'
    'am9-VXSNgPWV7BLylqL3sgHXB9YILtYdofp4EpCEHiVV-FQ-Wl3j8Dt_FUsWM68dRqp_0FRwHQpR9xxW'
    'AYYvh8jKcmRvUiVpCe8gcyzza7AhfkKM-2zRi0vJBNBrHsLyBZH7KiVhSlqY7fPEdFt86RfyltBSJQU8'
    'DSoQo1W6yBAgTDPcRd53Eh8l41E3oTad1OtiKSkWWZ3OmcKAvH8Ox8Q1xOBESRETL5-yrV8QmPzbakha'
    'wKELSvst5cYm6MeY0eRvFF58n-EXs5kYPeSisgQ0bM3wuAp-EFVwHyIPRXHoB9WxNSp55sK4R5wIcIHN'
    '8MYqfkDTdWB4DXErEIyIM8YgTs25oEYtzWbMROSXQeJ7WXgqBZAWSuyeti2iQSPxRKeIR6sAp8Dlc9Sg'
    'AoyIL57AWl4ANeZ5in0CPmClOREpm9JzIUrHqPGD57g0AyhRzjKXG-DMBA7z7eurzhifviDIIVYvWigV'
    'detk-b26Q5wvAaL17sqRnCkb8iiVKPeI3Js-Vb02tgVeJjUqvqUYyxHospQSHwaFmSDbryTqcXqEbsZe'
    'E24QD1SdNwMzwAHPZIx6biJD266f161jB7WQM9IrmzFcVwyEX0MZQqisv0qerFkifKDQDegxCsuPWJcU'
    'JUtv6EP_ZHQRKqKLHyl6W8VxSWHMGALEs9kChTVp8hbqECcNwC1pL1MiNWdpTgfbwnv5SF37CYOdy4oi'
    'Baq90Hab0ELov-K5GbENvROC7rkQoDhGm4bMmE4Q1djdhHTfzgW6zPhh-_2IoJktavTnV2bXNCwkugvy'
    'hRd_-i2zCak9P4YyBxctBlDDuvJ0ZPnOYBgiudU1xkQ337kH5qEEC3vF17r0hBN3nB8aCFD6Ce8E4OWK'
    'IkGAkaiYQBdcIdqB52jo18V5aJ06QwRD-RmJ_ElupmvRSjLiz4e92yF2n2MWbyKd_oyegSnnZ49pYwZP'
    'BpvC0jGsuKIAGdncUQUkPW_RqFZCQhho_gqMs9xv0phNJRcE_q0jSdrMjk48a6a_GS-6JpREZsD24PIV'
    'UtwBhrVk4eOdDeMxcflgBYGRuepSxL5o8JhcYe8FmpjxE8d5mG2Dh07P6exK0gZRIRipikDcgu1siZvH'
    'tvL6x8hC-nAy-eBizQZIidUxMjSQF00Ekn3ORjAi40WFs_RVBSRaGCpLjsH7yA7PvFLwMPYB_IPr7vhY'
    'ojsjpRb_E8PRSEr4_yxMRZP-1TuvhZUdyPRIm87cKI8I8tHmduYJF5UnewyQtjQMY9FE-GQgWfKxr4qT'
    'XGhzy5qFaRfx9lSRU-akiqcgHUmO7WW5fBM-bCUvh0jE-TIth7dXS-mvJLNogKnuICPSFysIuaH477UO'
    'QQcUaJqRIj3MSSeNXZxiyvVKEY6fYYF_oQd4NfYPpSrfgjilzwjCRmod5Z13xvGgzsP19loz6PN89Xyk'
    'ieZC2BAus-MdqkkQy8F9Bdr9bGJaPeUnjgkIpdF4yyF5QSKmFJWZGmQZl6jJbUOVCP2nNQXqtnBRSm6i'
    'ImJ1H86d2yeS0vW6FVvkyc-5mQH_4yH96v4tb1sxaaKOosXsgoKl74eqvbqa9VX7UecF0RYNXKZPHAgQ'
    'ZM_Tfpl_27y45RkDrfLX7ytx-Fr9vtqoEddeBue6v6D9-hfS-eCaZS8BRJAXaQ6pwoOD1w5KAjr5zlU0'
    '61PGSg4LvIPeag5CBMwAcZJHED9pG6vg5-EZRzP7MCcc_hjdetwZuU9fdV69NaB1QK5cL-U9XuVj-seP'
    '0bsRnOU7WuOSzybtVcq9besq4lVZlb5BcgNrib96rZ7fhwO34_2ZUulFixG7FkVKobh-DvCyg44ccxIn'
    'AhhCa7IuYXBuS4UHjMltuohf4RGbaxkC4-QbGGAkgn9-h2oRSzcrvuw3pztbG1BlvogVv8XtewEztw-0'
    'G9u357s2pvbHVr5PSy-0PXK1uqQRN9hWABI8IiHJYXmlX7sst6ZW2tfflqvfuwBP9PuYRv2q37DtkqW4'
    'gCfPgk80lBTa5tBewuaH3fD3Rl8H9VR7wRW7KV40eB8WAB33MR8AS26_8m2TrK4f8vKaB-5J9ryGuC9y'
    'YzHHPYntxMoaAyhjv2Inu63G3zwuqxw1KvBEECFO-e85tGbbVgAWVDtPAKFLoLIw-ysSEHiYhkYmO3jH'
    'MRXkwUGOYT-qxAAvhKQnFdKez7Z67L8pf-jV-FGZR6twwi7qm5tXEpqh4jHLSmlcbvrUxKVPvNpWrdX5'
    'VDKfyqlFSfIkUM6NlkGyxLyNAJj_0ezHiPJYZM6VBZpUc6O0z3lm0nU4VghQ_peCuQZidUkmcJdGcF26'
    'USDF-TGyEF-7z14JYaBrzjQv_5R5CWhSxSIwzOgEOWZsdt6yW0xHnveeuYnr-BTHZUjfE2fSuzXOlcy-'
    '613CGExyRuz5WFzLK_hTSVDa5JXIocN53jL11tJMVHvEdBYOFFiywciG67P3xQQTC-hb_723PhlEXp0h'
    'xxsqMXjtd0SBWjkCujxvqDqIdCQt13GKwP2MYNyWPPU8GeJom9lBj2IhnsSeK2nBvO_3_mgfZ5Mpp-Ri'
    'B5tj4qiSRf0x3MK3RGULB1F6AQuyzkEAqfw1zlXvnRNBB0InUjCmxAom2RS7ZvHNImMleFg1GaPkjBss'
    'Gyxq5u0DiFi7fbAGSRdbj0PiVlSebCJhiK3TfF_rrizOLSO-Olh6rK4dbQ8z8nu15kRiLYsBp8iOEFr5'
    'vJonI774ek25yrAYj6il1lnt6OyZnqCGW_Q3UAXABX68w1K3-NnOA9Cmswizsm2ysavMs1pSnbutp99X'
    'lzKZRcXuGXjDOaY1A5CPq8N_mlIUihaaRCeIEV0hHnSVoO5Xwx2UrAKABjZnY8znqdxgs3P_YGqbM4yv'
    'xzAlI71C4IKrqixJvgXQDlV6fm_uhVReKWgflyNcQRZcudz6va_YfWfOcy_76y9i-OuxB-NZVjeFxu_k'
    'PpIntdYsomJbofcB1mAE4Ew2oSQXFHJMaX8VDYbbsVYrof0RSOTeRHzC0VwBQJGprPyY40IhrhYxgQbl'
    'bX06B-fDIS-agLCohitAxw4Lfqz74ub-kVA-aQMntWlz2wzjaExYpSwFfOBMOSlsDgH1JuKnnChCgDEa'
    'BfEJ5gYFeR6u88vJg-MUvvSgzLs50ButHWQLXsjLzSaAYtXRCL-b_f--Re1fxHECfYdS6SsvuA05fD-R'
    '6b-Bs8wN0hsvI3Cx__Y5IqL_1_E5H3DyGxw9xMvdn5R-SVQgZq4ISqF2ChE-iW8t_oilE6dUEb4pkRnk'
    'ODagOC41RVObE55FsHtLJh7GJOOW4mIwyjyCiGcNfmFfcmIdNJ5vcS0U_5RBaXlDJWSYeMiGZuVWCPoj'
    '2KYS3o5YLJPFCvj5CYAfk5RtH7kJ_NFvJGkxYdieYHUiOUqAeylzmNrbxBWVQmzYCN-lRBAWssYmqfT-'
    'krZ5o4Nf8pUPTI_GitDXCoRS9RP2I55q2nqT8lmh5T-pDRK9mH6R7iK5d3nAvowrmEOJPizStcC_xw5R'
    'g-9TJr53pus5ofQpFhidSwQJWEzyB8Gu3jA0J4X2SdHcfGuj2OnA09-CArJI1Rl1fkYC0J_dSe0AaF03'
    'SZFHcdZMUOsH8-W2LJCTBspHB5mM5GJnP2mStn2YsJ2QUjM6BoJjX9jek0qBn94skjr4AWus-gBND3Wi'
    'SyWN8q-wajZ9uzkZDiCqDSufHTpDWGIpiMB3NJpPE0yiSZl0983e3rSsjfE3K69JMZXO8H6me2xQEdcm'
    '9uFT7ofZUI-PhIowAqUsHc275eE8pYToJPaAnaF-hYjRlbLxZFjTpa7WfMR27zE_GQra5XVLlo062Jac'
    '7qnJNG5OBYr050UMSjkaq5YfJj5vTZm5-_P8ZSRjWCd0cKej02igr8WoX6G2k8FG6UFnMEeIUVQIzcvX'
    'vW2ADDpKri49PsrId7Su7InRu5vHnBXm0uZdgGfUjpm5ukM2Nh5o95bWJc6H1xGmA_LR7WKUqCOk_Y0O'
    'ueEiVPSLtP7_ur5FApJasiA1O9PgaELicq-77UgJG3ZPSAYVKfOWn8Z8SNPgNmPnOCV3JEpecW42dzDl'
    'JgiueYlIKDeo1RL44HJYLglKe4A6fEylKaUg3mCIN6MEIpboZVq7P1GdVrjwKpVfOrpV5RH5Dz_J-MHv'
    'q45DPW6NN6DcwlyuY1ZnpcSjjvXO7x7UBk7bHUHj-WlxfGFAVluEAK3gM0aMJ84idY65vPN6S4i4GH-C'
    'fd2fGRdduoB970bwUrxOjLQ-Yc_dBV6dh6YHouy-NDnpscRwI0dcChWBEXgpXiuBgcQpoHkILhvhf_kd'
    'YkXIZYoOVvQ7fUrN9c8iLr5agSKjEwjZ5rWAwDeeR98kS8vTsGIWwnff_-ct6CaDqzeAklVBdTungxsJ'
    'EXDWI550xTmlj0PmkKngK9ArJXkxc_hVQungOoz4cnzk8rZSnfgYWyTlQGOk5i8WkBinaJ8gFb0MKGRy'
    'PYJ96gLzkdf-TLizRpTvRxkgGx0mjxpB6t22nLiceMcNtPDBi9VovIHGzj3RoH-SGINYVabWiFro6kQB'
    'vQIZZOiupIaRGIPOiavSAxooK9RBNqyvvjmPxJvkFNiTdwdeISPuXUpwipIy5yoKxn9MQZhXQjxbgHTE'
    'GRGJnV16gwDvR80PMuCrzOcRT_JsVN6SHddaNm0NScFOkyTPqUvG8WQlRMlXMlkzXTxRgH1d1JevETSD'
    'poqGPdPUYnPGEEjUe6RtPzxAFKN5_DSrrx9WdfL3U2a-3vG9XyJyJwH5Jor7FOyt3eM8eKXBBE1dFiYS'
    'W-swXvuj5o0ORFkPJrH0hMA8hDkcoSPEJ4aAiaybaKQkk0JsWnkJXvFtOhXvKhtXusns914TsiJ0F3h2'
    'g9eDo--G4RpGbfLWqY2hI45EGxNPFkFPaYCp5N65kEzzuq3-B34gUTqxCa13_zXou8D5K_MUU8-ElEYS'
    'V-Jzy6_CyiLwW7oxC_9DEfPoHHI3vFnraVJp0gmVMTnzDyyt6RS6_I3W3458CyinHAg5DdsTPJ8-89oq'
    'Ep-MTOouV0NNeIhtTx0zPIMXXAtp3C7DorFldmKOMQOJssgKIHEWHoAguBqoLZf43SrSMUdgUa9ZeoVB'
    'Sfo_zhzytWWo-bPJNncYV6YdoITR9J5ekI9ezcmcCuBcyxi26JibmYVT5iEVoMDjLINJSQFZMnLdBlTl'
    '0BSfjAUIMqOJ9I0pQd9-M4vB57V1hUoepkTJQ-EnzREQcheY2ScTpE_YVCRPzyB1Y0XvQL3N893D6McR'
    'CieDRU4MXsHsAcvmzAG0eRHYfwKELcFD_the1L_xHPSkRdJJWY_tCTdx4fqAsu3GdTk5PAeWyuIdyRKd'
    '5I6LqPw4uE7499opDFmovyDZTtLbAnw2ir0qe55xMQbGfohYHOufGpVxZhS1sfWZcQDR0lw5RXYwwwFB'
    '0lw51m6fKK0QRElmdlwcDDo2iasCw7Yg5orYK5T5o5uOvPaDnSU4h_4FScqd9fcQ0qR3_hoPXHpVcaRo'
    'OG7SD3kw6k6N34hK1YBa9IrP44mPDJk0Y6d_cxsP0A8qEZuTxYroKcCycJIQfFkomSTToNyacGvRwwl_'
    '_pw7VGS_P4cJmVnAd9qmAYBBomoPjxWmiM_7bRqG926h3dDK0ukmyQyHU8WIlpQL5fR83SX0L2SAl-Jq'
    'eJfdyni2SPgNa-U9w4wrK7mT6suEWyNP1dJapkIIMj8yKftxRzI0uvyS54DVCnPerhr_irVr25Ng3nAx'
    'VSwas15_OCHPAZXCHimgbauyM0bpGxeErXazzO2aI7rqddjowb5ZpIm155lbGYF64lryhyss5_xAzMI7'
    '6wjj5GGf4xcbY25zE4lEG9CwnGXY18ApwNH7O7tCM8BJKcFPhUAIGZCAcFeqyMVhMnhARQg19i5Gm63J'
    'H91VCGTgF6F7inlyge2dE5BqwKwg5GUSWQ8nyM3esrKTFe7tCgyoynp6j-E5g6lOvr9h7zXVcOA44OMG'
    'YRgO4hUUsssxiiLZbWx_ZBM9E5Rp8vsfmc6GiVjTCLeUgdDDcKeQw4R063-Mr56LQ8KafjzJIfn0Ke4O'
    '7skb-WlXuJg00reRSCXaSt9nmetr0QIqv6Dv1_PO7zJd-3Oi3_tn_3hVIzcqPpXJSYzMrNfrCm5_Ixd2'
    'FEhzP7t-8vneFN4jz5vYxxZt4JWcbCFWF63qE3i-EZm5ITKArS1-n5iEpR3KHTc_aO9s_uTGyP6GYsVt'
    'i0vMNz5hsFFeiTWXnx5fDVOVbY0TnWuL96nHeUjolTQ1Phcq6TsXdpMRtLvBXRi_qsnNjzk3PU6V9mzr'
    'u76CrydrmJa907xz6bPafeRSOpEbw6R-7jc4zXGdZ5iHeyGTnPNwNewH-enGDhPnummJvszxpDPNYpPu'
    'TstpkFIAowMy-xAgfnGVFS_aY9MyPnDl8rZtdJzhFm9X9n9ipCedPMU8r9fMFiu-n5qK6X56HL5GHL5d'
    '07x4i_pFWEJ9WUehcx7A6aCJZ4Zy-e6mtmkvxVP2K26ADF0qdxBkfhWSZ6OCTL-gm6IOx7b0dCccM19F'
    'RIKkSOfqzYcJUzco10Bh62oULGl7IA3AsnRpmBGYoROaHQTLdOMixI34An5PH6jNw0EHrvkjfTHPEb5k'
    'oRfM_raSTZAFlb7k3IZEYz5uPg40JXluaEd37UriRvCtMuA0Mkrpl8Zt2Q_M47AAGSsM-v5iPZAzpXRU'
    'XZAaNq4i3QbdHXHiGblgJekbgA_9-r9LCoXowMjOL_yKx9Iqxg3-DX3y_EPeol59br5Jdwwc_lIIHSmV'
    'A1wpbkMDKA2MJDlTCR6nMor0lngWfWXB1JQvfYXna9yxIheyF27NV4qNs-4nrD9YBCsspr5dg6QGbfbE'
    'BhVF0nLIuSu54PDRIYirxey-nlAh7Smf4yp1FKLoCH6O-_T-8UoioH3KdZSkIJC_LGh_y2XrQM4pso4A'
    'yCbJRDwjL4SeWe_BgL9nXVJMuIAPAp5Jbhqd5r5BdanTv9ySMzfvzO-go1DXajeDCISpWIAmf4AB3AHa'
    'GeiL7J03DRWIrz1CGoukbKs0cJ8UU7CSB7mbpt0mhqknz62eV7Kzqj9kASiMJGO8PTNpwSj3mG-1r88A'
    'QVaBQt3GedAA1yc_uToc2TXwJawyxx6qNLIXXEvfS6UT5WwCILzWKYAKkEK8-iw2ySjcmpneQmGhyqia'
    'G0OYpebkFd5E2mAqX-6vO7Hr0S8luFpzCnNyowqYZLkCUmKUtR89NrbrIaqFGUWRRy00KsoexD00pR-f'
    '28iu-5fDrrtCoNUjL8UK5a2wdRdWzxaeENzRt6Zzzgnj-41HlVOmepc-KSSZwa3_qPjxYf1P-t1l5b_O'
    'RurdGp_uP_AaVzLmAifwEA'
)


@pytest.mark.scenario(name='Weather', handle='weather')
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('hw_weather_change', BG_GRANET_SOURCE_TEXT)
class TestsWeatherChange:

    def test_when_begins(self, alice):
        r = alice(voice('когда будет дождь в москве'))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0
        return r.run_response.ResponseBody.Layout.OutputSpeech

    def test_when_ends(self, alice):
        r = alice(voice('когда закончится дождь в москве'))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0
        return r.run_response.ResponseBody.Layout.OutputSpeech

    def test_begins_tomorrow(self, alice):
        r = alice(voice('завтра будет ли дождь в москве'))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0
        return r.run_response.ResponseBody.Layout.OutputSpeech

    def test_begins_next_week(self, alice):
        r = alice(voice('будет ли дождь в москве на следующей неделе'))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0
        return r.run_response.ResponseBody.Layout.OutputSpeech
