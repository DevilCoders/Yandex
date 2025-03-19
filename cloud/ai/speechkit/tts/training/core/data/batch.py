import torch


class Batch:
    def __init__(self, batch_size: int):
        self.batch_size = batch_size

    def to(self, *args, **kwargs):
        for attr_name in dir(self):
            attr_value = self.__getattribute__(attr_name)
            if attr_name.startswith("_") and isinstance(attr_value, torch.Tensor):
                self.__setattr__(attr_name, attr_value.to(*args, **kwargs))
        return self

    def pin_memory(self):
        for attr_name in dir(self):
            attr_value = self.__getattribute__(attr_name)
            if attr_name.startswith("_") and isinstance(attr_value, torch.Tensor):
                self.__setattr__(attr_name, attr_value.pin_memory())
        return self

    def __len__(self):
        return self.batch_size
