import numpy as np
from tools.audio import AudioSample


def normalize(audio: AudioSample):
    max_peak = np.max(np.abs(audio.data))
    should_max = 0.70794576  # magic constant from sox

    if max_peak == should_max:
        return audio

    data = audio.data.copy()
    data = data / max_peak * should_max
    return AudioSample(data=data, sample_rate=audio.sample_rate)
