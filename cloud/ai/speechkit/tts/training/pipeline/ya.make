OWNER(g:cloud-asr)

PY3_LIBRARY()

PY_SRCS(
    audio_lowpass_filter.py
    audio_noise_reduction.py
    extract_mel.py
    extract_pitch.py
    extract_pitch_old.py
    normalize_audio.py
)

NO_CHECK_IMPORTS()

END()
