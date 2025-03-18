import yatest.common


CASES = ("gen", "dat", "acc", "ins", "abl")


def inflect_in_case(input_file, lang=None, grams=CASES):
    command = [
        yatest.common.binary_path("tools/inflector/inflector"),
    ]
    if lang:
        command += ["-l", lang]

    in_path = yatest.common.source_path("tools/inflector/tests/" + input_file)
    out_path = input_file + ".out"
    with open(out_path, "w") as out:
        for gr in grams:
            with open(in_path) as std_in:
                yatest.common.execute(command + ["-g", gr], stdin=std_in, stdout=out)

    return yatest.common.canonical_file(out_path)


def test_kemerovo_stations():
    return inflect_in_case("kemerovo_stations.txt", "ru")


def test_general():
    return inflect_in_case("general.txt", "ru")


def test_tricky():
    return inflect_in_case("tricky.txt", lang="ru", grams=["gen,pl"])


def test_streets():
    return inflect_in_case("streets.txt", lang="ru", grams=["acc"])


def test_indirect():
    return inflect_in_case("indirect.txt", lang="ru", grams=["nom"])


def test_names():
    return inflect_in_case("names.txt", "ru")
