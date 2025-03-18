import re

replRe = re.compile('[<>\'\"]')

def Escape(s):
    def Repl(m):
        d = {
            '<': '&lsaquo;',
            '>': '&rsaquo;',
            '"': '&quot;',
            "'": '&apos;'
            }

        r = d.get(m.group(0))
        if r:
            return r

        return m.group(0)

    return replRe.sub(Repl, s)
