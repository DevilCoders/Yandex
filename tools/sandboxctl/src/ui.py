import io
from lxml import etree as ET
from yalibrary.yandex import sandbox


def html2text(html_str):
    if not html_str:
        return ""

    ret_str = ""
    parser = ET.HTMLParser()
    tree = ET.parse(io.StringIO(html_str), parser)
    for el in tree.getroot().getiterator():
        text = el.text
        href = el.get('href') or ""
        if text:
            if href:
                if text.endswith('\n'):
                    text = "%s[%s]\n" % (text.strip(), href)
                else:
                    text = "%s[%s]" % (text, href)
            ret_str += "%s %s" % (text, el.tail)
    return ret_str


def get_task_state_printer(dspl):
    TIME_PRECISION = 2

    def printer(time_, task):

        def format_msg(color, body=''):
            return '[[unimp]][{} s][[rst]] [[c:{}]]{}[[rst]] {}'.format(round(time_, TIME_PRECISION), color, status, body)

        status = task['status']
        body = ''
        client = task.get('execution', {}).get('client', {}).get('id', '')
        if client:
            body = ' [%s]' % client

        if 'results' in task and 'info' in task['results'] and task['results']['info'] is not None:
            info = task['results']['info'].splitlines()
            body = "[{line_no}] {line}".format(line_no=len(info), line=html2text(info[-1]))
        elif 'tails' in task and task['tails'] is not None and len(task['tails']):
            body += " {}".format(task['tails'][-1]['url'])

        if status in sandbox.SUSPENDED_STATUSES:
            url = ' [[c:yellow]] WEB_SHELL_URL: {}[[rst]]\n'.format(task['execution']['shell_url'])
            dspl.emit_status(format_msg('red', url))
        elif status not in sandbox.DONE_STATUSES:
            dspl.emit_status(format_msg('yellow'), body)
        elif status in sandbox.SUCCESSFULLY_DONE_STATUSES:
            dspl.emit_message(format_msg('green'))
        else:
            dspl.emit_message(format_msg('red'))

    return printer
