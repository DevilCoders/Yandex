#!/usr/bin/env python
# -*- coding: utf8 -*-
_FIELDS = {}
_FIELDS["common"] = ["id", "query", "region", "url", "relevance", "title"]
_FIELDS["candidate"] = _FIELDS["common"] + ["snippet", "algo", "rank", "coords", "features"]
_FIELDS["assessed"] = _FIELDS["candidate"] + ["marks", "assessor", "timestamp"]
def getFieldIndex(field, type = "assessed"):
    return _FIELDS[type].index(field) if field in _FIELDS[type] else None

def _printed(param):
    if isinstance(param, dict):
        return ' '.join([str(key) + ':' + str(value) for key, value in param.iteritems()])
    if isinstance(param, list):
        return ' '.join([str(value) for value in param])
    return str(param)
        
class SnippetsData:
    def __init__(self, data = None):
        self.id = -1
        self.query = ""
        self.region = ""
        self.url = ""
        self.relevance = 0
        self.title = ""
        self.snippet = ""
        self.algorithm = ""
        self.coords = []
        self.scoords = ""
        self.rank = 0
        self.sfeatures = ""
        self.features = {}
        self.featnames = []
        self.assessor = ""
        self.smarks = ""
        self.marks = {}
        self.timestamp = ""
        
        self.fullstring = ""
        if data:
            self.Load(data)
        
    def LoadSimple(self, data, Nparam = None):
        self.fullstring = data.strip()
        fields = self.fullstring.split("\t")
        type = 'assessed'
        if Nparam == None:
            Nparam = len(fields)
            if Nparam < len(_FIELDS["common"]):
                return 'SHORT'
            elif Nparam < len(_FIELDS["candidate"]):
                type = 'common'
            elif Nparam < len(_FIELDS["assessed"]):
                type = 'candidate'
            elif Nparam > len(_FIELDS["assessed"]):
                return 'LONG'
            
        def fieldvalue(fieldname):
            fieldindex = getFieldIndex(fieldname, type)
            return fields[fieldindex].strip() if (fieldindex!=None and fieldindex < Nparam) else ""
            
        self.id = fieldvalue("id")
        self.query = fieldvalue("query")
        self.region = fieldvalue("region")
        self.url = fieldvalue("url")
        self.relevance = fieldvalue("relevance")
        self.title = fieldvalue("title")
        self.snippet = fieldvalue("snippet")
        self.algorithm = fieldvalue("algo")
        self.rank = fieldvalue("rank")
        self.scoords = fieldvalue("coords")
        self.sfeatures = fieldvalue("features")
        self.assessor = fieldvalue("assessor")
        self.smarks = fieldvalue("marks")
        self.timestamp = fieldvalue("timestamp")
        return type
        
    def Load(self, data, Nparam = None):
        dataType = self.LoadSimple(data, Nparam)
        if dataType is 'SHORT' or dataType is 'LONG':
            print 'Error! Data file format is too', dataType
            return False
        if dataType is 'common':
            return True
        
        self.coords = [ x.split("-") for x in self.scoords.split(" ")]
        self.ParseFeaturesString()
        if dataType is 'candidate':
            return True
        
        marks = [(mark.strip().split(":")[0], float(mark.strip().split(":")[1])) for mark in self.smarks.split(" ")]
        self.marks = dict(marks) if self.smarks else {}
        return True
        
    def ParseFeaturesString(self):
        self.features, self.featnames = SnippetsData.ParseFeaturesStringInternal(self.sfeatures)
        
    @staticmethod        
    def ParseFeaturesStringInternal(sfeatures):
        features = {}
        featnames = []
        for feat in sfeatures.strip().replace(' :', ':').split(" "):
            feat = feat.split(":")
            if len(feat) != 2:
                print 'Error! Wrong features format'
                return False
            features[feat[0]] = float(feat[1])
            featnames.append(feat[0])
            
        return features, featnames
    
    def GetSourceString(self, type="candidate"):
        fields = {}
        fields["id"] = self.id
        fields["query"] = self.query
        fields["region"] = self.region
        fields["url"] = self.url
        fields["relevance"] = self.relevance
        fields["title"] = self.title
        fields["snippet"] = self.snippet
        fields["algo"] = self.algorithm
        fields["rank"] = self.rank
        fields["coords"] = ["-".join(map(str,coord)) for coord in self.coords]
        fields["features"] = [name + ':' + str(self.features[name]) for name in self.featnames]
        fields["assessor"] = self.assessor
        fields["marks"] = self.marks
        fields["timestamp"] = self.timestamp
        return '\t'.join([_printed(fields[value]) for value in _FIELDS[type]])
        

if __name__ == "__main__":
    example = '''100526340
HRITHIK ROSHAN
2
http://www.hrithikroshanweb.com
10
<strong>Hrithik</strong> <strong>Roshan</strong> - Photos, Wallpapers, News, Biography...  ...of them got success from this movie.
<strong>Hrithik</strong> <strong>Roshan</strong> and Amisha Patel have done the best in the movie. He won the Filmfare Award for Best Actor and the Filmfare Award for Best Debut also.
Algo2
19
83-117
qual:0.1 dots:-1.5 tgaps:-0 sgaps:-0 nav:-0 lanchp:-0 hanchp:-0 shorts:-0 idf:1 uidf:1 rep2:-0.16 sent:-0.2 userw:0 userlw:0 pos:0 userl:0 lemm:0 good:-0 exct:2 seq:2 langm:-0 len:0.1987743613 tsim:-0.01632653061 end:-9e-05 ledge:-0 redge:-0.017 pbeg:0.1 llangm:-0 footer:0 content:0 mcontent:0 header:0 mheader:0 menu:0 referat:0 aux:0 links:0 llinks_lp:0 llinks_up:0 llinks_avp:0 domains:0 inputs:0 blocks:0 link_words_l:0 link_words_u:0 link_words_av:0 ads_css:0 has_header:0 poll_css:0 menu_css:0 rdots:-0 widf:0 userws:0 userwns:2 wizws:0 wizwns:0 userlws:0 userlwns:2 wizlws:0 wizlwns:0 ursrls:0 ursrls:2 wizls:0 wizlns:0 ubm25:0.01523631074 len_0:0 len_50:0 len_100:0 len_150:0 len_200:1 len_250:0 dom2:1 extern:0 info:0.1 mtch:0.07 luporn:0
content:1 readability:2
A-Babina
1278154468'''.replace('\n', '\t')
    print 'Source string:\n\n', example
    d = SnippetsData(example)
    dd = SnippetsData()
    dd.Load(example, 4)
    dd.Load(example, 13)
    print '\n\nProcessed string:\n\n', d.GetSourceString('assessed')
