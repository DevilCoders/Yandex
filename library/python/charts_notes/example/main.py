import library.python.charts_notes as notes


def main():
    note_id = notes.create(
        feed='Users/voidex/offline_eta',
        date='2019-02-25',
        note=notes.Line('lorem ipsum', color=notes.Color.GREEN),
    )
    dump_notes('created')
    notes.modify(
        note_id=note_id,
        note=notes.Line('lorem ipsum modified', color=notes.Color.RED),
    )
    dump_notes('modified')
    notes.delete(note_id)
    dump_notes('deleted')


def dump_notes(title):
    print('----------')
    print(title)
    print('----------')
    for n in notes.fetch('Users/voidex/offline_eta', date_from='2019-02-22', date_to='2019-03-01'):
        print(n)
