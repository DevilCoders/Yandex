from typing import List


class Callback:
    def on_train_start(self, trainer, module):
        pass

    def on_train_end(self, trainer, module):
        pass

    def on_train_batch_start(self, trainer, module, batch, batch_idx: int):
        pass

    def on_train_batch_end(self, trainer, module, batch, batch_idx: int):
        pass

    def on_validation_start(self, trainer, module):
        pass

    def on_validation_end(self, trainer, module):
        pass

    def on_validation_batch_start(self, trainer, module, batch, batch_idx: int):
        pass

    def on_validation_batch_end(self, trainer, module, batch, batch_idx: int):
        pass


class CallbackCollection(Callback):
    def __init__(self, callbacks: List[Callback]):
        self._callbacks = callbacks

    def on_train_start(self, trainer, module):
        for callback in self._callbacks:
            callback.on_train_start(trainer, module)

    def on_train_end(self, trainer, module):
        for callback in self._callbacks:
            callback.on_train_end(trainer, module)

    def on_train_batch_start(self, trainer, module, batch, batch_idx):
        for callback in self._callbacks:
            callback.on_train_batch_start(trainer, module, batch, batch_idx)

    def on_train_batch_end(self, trainer, module, batch, batch_idx):
        for callback in self._callbacks:
            callback.on_train_batch_end(trainer, module, batch, batch_idx)

    def on_validation_start(self, trainer, module):
        for callback in self._callbacks:
            callback.on_validation_start(trainer, module)

    def on_validation_end(self, trainer, module):
        for callback in self._callbacks:
            callback.on_validation_end(trainer, module)

    def on_validation_batch_start(self, trainer, module, batch, batch_idx):
        for callback in self._callbacks:
            callback.on_validation_batch_start(trainer, module, batch, batch_idx)

    def on_validation_batch_end(self, trainer, module, batch, batch_idx):
        for callback in self._callbacks:
            callback.on_validation_batch_end(trainer, module, batch, batch_idx)
