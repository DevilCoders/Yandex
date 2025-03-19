import sys

out = sys.argv[1]

with open(out, 'r') as f:
    for l in f:
        l = l.strip()

        if l.startswith('//'):
            out = l[2:].strip().lower()
            out = 'seinfo_ut_' + out.replace('.', '_').replace(' ', '_').replace('-', '_').replace("'", '_').replace('+', '_').replace('@', '_').replace('(', '_').replace(')', '_').replace('__', '_').replace('__', '_') + '.cpp'
            out = out.replace('_.', '.')

            break

print out
