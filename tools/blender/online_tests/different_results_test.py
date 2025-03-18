import urllib2
import json
import time
import urllib


class BlenderOnlineTest:
    def report_name():
        raise NotImplementedError()

    def test_name():
        raise NotImplementedError()

    def run():
        raise NotImplementedError()


class DifferentNumberOfResults(BlenderOnlineTest):
    def __init__(self, src, q):
        self.src = src
        self.queries = q
        self.params = '&test-id=1&lr=213&waitall=da&nocache=da&json_dump=searchdata.docs.*.url'
        self.num_doc = [10, 20]

    def test_name(self):
        return '10_20_diff'

    def report_name(self):
        return self.test_name() + '.txt'

    def run(self):
        diff = self.get_queries_with_diff()
        if diff is not None:
            self.make_report(diff)
            return False
        return True

    def get_queries_with_diff(self):
        res = []
        i = 0;
        for q in self.queries:
            i += 1
            diff = self.has_diff(q)
            if diff is not None:
                res.append([q, diff])
            #time.sleep(0.1)
            print 'processed', i, ' of ', len(self.queries), ' queries'

        return res if len(res) > 0 else None

    def has_diff(self, q):
        try:
            arr = []
            for numdoc in self.num_doc:
                url = self.form_url(q, numdoc)
                response = urllib2.urlopen(url)
                html = response.read()
                arr.append(get_docs(html))

            length = len(min(arr))
            for i in range(0, length):
                if arr[0][i] != arr[1][i]:
                    return arr
        except:
            print 'error in query', q


    def form_url(self, q, numdoc):
        return 'http://' + self.src + '/yandsearch?text=' + urllib.quote(q, '') + self.params + '&numdoc=' + str(numdoc)

    def make_report(self, res):
        with open(self.report_name(), 'w') as f:
            if res is not None:
                for q, diff in res:
                    f.write('URLS:\n')
                    for num in self.num_doc:
                        f.write(self.form_url(q, num) + '\n')
                    f.write('SERPS:\n')
                    length = len(min(diff, key=len))
                    for i in range(0, length):
                        char = '   ' if diff[0][i] == diff[1][i] else '!!!'
                        f.write('%i)\t%s\t%s <---> %s\n' % (i, char, diff[0][i], diff[1][i]))
                    f.write('\n')
                f.write('\n')


def get_docs(html):
    return [d for d in json.loads(html, 'utf-8')['searchdata.docs.*.url']]
