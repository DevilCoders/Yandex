import math
import typing

from cloud.ai.speechkit.stt.lib.data_pipeline.transcription_tasks import get_bit_overlap
from cloud.ai.speechkit.stt.lib.data_pipeline.records_splitting import get_bits_indices

toloka_commission = 0.2

min_cost_duration_sum = 60 * 60
min_cost_record_duration = 9

min_cost_records_count = min_cost_duration_sum // min_cost_record_duration
min_cost_records_durations = [min_cost_record_duration] * min_cost_records_count
min_cost_records_channel_counts = [1] * min_cost_records_count

"""
При расчете стоимости разметки для партнеров мы отталкиваемся от себестоимости разметки.

Себестоимость разметки вычисляем на основе суммарной стоимости страниц заданий транскрипции,
которые нужно будет выполнить для разметки переданных записей. Двухуровневая схема разметки
с предварительной проверкой транскрипцией ASR не учитывается.

Биллинг юнит разметки соответствует одной странице заданий.

Число необходимых страниц заданий транскрипции вычисляется так:
1. Считаем, сколько фрагментов получится при нарезке исходных записей.
2. Учитываем перекрытие фрагментов на краях.
3. Учитываем, если запись многоканальная.
4. Делим итоговое число "виртуальных" фрагментов на число заданий на странице заданий.

Это даст приблизительное число страниц заданий, не учитывающее крайние случаи с малым числом
фрагментов (например, на странице заданий – 20 заданий, мы размечаем 18 фрагментов 1 раз
и один фрагмент 3 раза, на самом деле в этом случае придется создать 3 страницы заданий, тогда
как наша формула вернет 1 страницу заданий).

Учитывая предыдущее, а также ненулевые расходы на инфраструктуру при запуске разметки,
мы решили задать минимальную стоимость разметки, примерно равную разметке 1 часа аудио,
когда каждая запись равна 9 секундам.

Для расчета тарифа биллинг юнита мы используем следующие значения:
1. Стоимость страницы задания транскрипции в $.
2. Комиссия Толоки.
3. Некоторый курс $ в рублях.
3. Добавочная стоимость.

В DataSphere передаются биллинг юниты, а их тариф вычисляется единожды
(с возможным перерасчетом в будущем).
"""


def calculate_final_billing_units(
    records_durations: typing.List[int],
    records_channel_counts: typing.List[int],
    bit_length: int,
    bit_offset: int,
    chunk_overlap: int,
    edge_bits_full_overlap: bool,
    real_tasks_per_assignment: int,
) -> int:
    real_units, min_units = (
        calculate_billing_units(
            durations,
            channel_counts,
            bit_length,
            bit_offset,
            chunk_overlap,
            edge_bits_full_overlap,
            real_tasks_per_assignment,
        )
        for durations, channel_counts in (
            (records_durations, records_channel_counts),
            (min_cost_records_durations, min_cost_records_channel_counts),
        )
    )

    return max(real_units, min_units)


def calculate_billing_units(
    records_durations: typing.List[int],
    records_channel_counts: typing.List[int],
    bit_length: int,
    bit_offset: int,
    chunk_overlap: int,
    edge_bits_full_overlap: bool,
    real_tasks_per_assignment: int,
) -> int:
    bits_count = calculate_bits_count(
        records_durations, records_channel_counts, bit_length, bit_offset, chunk_overlap, edge_bits_full_overlap,
    )
    return calculate_assignments_count(bits_count, real_tasks_per_assignment)


def calculate_bits_count(
    records_durations: typing.List[int],
    records_channel_counts: typing.List[int],
    bit_length: int,
    bit_offset: int,
    chunk_overlap: int,
    edge_bits_full_overlap: bool,
) -> int:
    assert len(records_durations) == len(records_channel_counts)
    basic_overlap = bit_length // bit_offset
    total_bits_count = 0
    for duration, channel_count in zip(records_durations, records_channel_counts):
        bits_count = len(get_bits_indices(duration, bit_length, bit_offset))
        for bit_index in range(bits_count):
            bit_overlap = get_bit_overlap(bit_index, bits_count, chunk_overlap, basic_overlap, edge_bits_full_overlap)
            total_bits_count += bit_overlap * channel_count
    return total_bits_count


def calculate_assignments_count(bits_count: int, real_tasks_per_assignment: int) -> int:
    return int(math.ceil(bits_count / real_tasks_per_assignment))


def calculate_billing_unit_rate(
    reward_per_assignment_usd: float,
    rub_in_usd: float,
    cost_multiplier: float,
):
    assignment_prime_cost_usd = reward_per_assignment_usd * (1 + toloka_commission)
    assignment_prime_cost_rub = assignment_prime_cost_usd * rub_in_usd
    return assignment_prime_cost_rub * cost_multiplier
