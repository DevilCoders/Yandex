lines = []
cnt = 0

with open('qw.cpp', 'r') as f:
    for l in f:
        lines.append(l)

        if l.startswith('        }'):
            cnt += 1

            #print '    ' + 'seinfo_ut_' + str(cnt) + '.cpp'
            print '        extern void TestSeFunc' + str(cnt) + '(); TestSeFunc' + str(cnt) + '();';

            with open('seinfo_ut_' + str(cnt) + '.cpp', 'w') as ff:
                ff.write('#include "seinfo_ut.h"\n\nusing namespace NSe;\n\n')
                ff.write('void TestSeFunc' + str(cnt) + '() {\n')
                ff.write(''.join(lines))
                ff.write('}\n')

            lines = []
