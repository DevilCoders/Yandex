import random


class RandomDropPostprocessor(object):
    def __init__(self, drop_chance=0.1):
        self.drop_chance = drop_chance

    def name(self):
        return "RandomDrop(chance={:.2f})".format(self.drop_chance)

    def process_experiment(self, exp):
        for row in exp:
            if random.random() > self.drop_chance:
                exp.write_row(row)
