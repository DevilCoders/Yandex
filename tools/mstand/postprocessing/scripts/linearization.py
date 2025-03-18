#!/usr/bin/env python
# -*- coding: utf-8 -*-


class Linearization(object):
    @staticmethod
    def process_observation(obs):
        control_sum = 0.0
        control_count = 0
        for row in obs.control:
            control_sum += sum(row)
            control_count += len(row)

        control_avg = float(control_sum) / control_count if control_count else 0.0

        for exp in obs.all_experiments():
            for row in exp:
                exp.write_value((sum(row) + (1000 - len(row)) * control_avg) / 1000)
