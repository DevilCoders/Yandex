import sys
import csv

assert sys.version_info.major == 3


GRAMMEMS = [
    "nom,sg",
    "gen,sg",
    "dat,sg",
    "acc,sg",
    "ins,sg",
    "abl,sg",
    "nom,pl",
    "gen,pl",
    "dat,pl",
    "acc,pl",
    "ins,pl",
    "abl,pl",
]

LINE_TEMPLATE = '[{stem}]{flex}\tS,famn,{gender},{gram}'


def get_stem(words):
    for i in range(len(words[0]), -1, -1):
        if all(w[:i] == words[0][:i] for w in words):
            return words[0][:i]
    return ''


def main():
    gender = sys.argv[1]
    for line in csv.reader(sys.stdin):
        if line[-1]:
            continue
        forms = [w.lower() for w in line[:-1]]
        stem = get_stem(forms)
        assert stem
        sys.stdout.write('%s\n' % (forms[0], ))
        for form, gram in zip(forms, GRAMMEMS):
            sys.stdout.write(LINE_TEMPLATE.format(stem=stem, flex=form[len(stem):], gender=gender, gram=gram) + '\n')
        sys.stdout.write('\n')


if __name__ == "__main__":
    main()
