IF (NOT AUTOCHECK)

OWNER(eranik g:cloud-asr)

UNION()

FILES(
    am_config.json
    config.json
    features_config.json
    letters.lst
    version.xml
    asr_system_config.json
)

FROM_SANDBOX(FILE 1519738559 OUT words.lst)
FROM_SANDBOX(FILE 1519737544 OUT lm)

FROM_SANDBOX(
    1501865427 OUT
    jasper10x5_trt_v100/jasper_fp16_bs80_128.trt
    jasper10x5_trt_v100/jasper_fp16_bs40_256.trt
    jasper10x5_trt_v100/jasper_fp16_bs20_512.trt
    jasper10x5_trt_v100/jasper_fp16_bs16_768.trt
    jasper10x5_trt_v100/jasper_fp16_bs16_1024.trt
    jasper10x5_trt_v100/jasper_fp16_bs8_2048.trt
    jasper10x5_trt_v100/jasper_fp16_bs8_3072.trt
)

END()

ENDIF()
