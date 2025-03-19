from scipy.signal import resample
from scipy.io import wavfile
import io
import numpy as np
import soundfile as sf


PROJECT_SAMPLE_RATE = 22050


class AudioSample:
    def __init__(self, file=None, data=None, sample_rate=None):
        if (file is None) == (data is None or sample_rate is None):
            raise Exception("Initialization could be done either from file or from numpy.array not together")
        if (data is None) != (sample_rate is None):
            raise Exception("If data is passed sample_rate should be passed too")

        if file is not None:
            self.data, self.sample_rate = sf.read(file, dtype=np.float32)
        else:
            if data.dtype != np.float32:
                raise Exception("Data shoud be in np.float32 format")

            self.data = data
            self.sample_rate = sample_rate

    def resample(self, new_rate):
        if new_rate == self.sample_rate:
            return

        number_of_samples = round(self.data.shape[0] * float(new_rate) / self.sample_rate)
        self.data = np.array(resample(self.data, number_of_samples), dtype=np.float32)
        self.sample_rate = new_rate

    @property
    def duration(self):
        return self.data.shape[0] / self.sample_rate

    def play(self, normalize=False):
        from IPython.display import Audio, display
        import time

        data = self.data
        if not normalize:
            data = np.clip(self.data, -1, 1)

        audio = Audio(data, rate=self.sample_rate, autoplay=True, normalize=normalize)
        display(audio)
        time.sleep(self.duration)

    def save(self, path):
        wavfile.write(path, self.sample_rate, self.data)

    def save_to_bytes(self):
        wav_bytes = io.BytesIO()
        self.save(wav_bytes)
        wav_bytes.seek(0)
        return wav_bytes.read()
