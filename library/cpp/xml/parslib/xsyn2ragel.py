#!/usr/local/bin/python

tab = "    "

def nm(s):
    return s.replace(":", "_").replace("-", "_")

class compiler:

    def __init__(self):
        self.elem = 0
        self.elactions = { }
        self.atactions = { }
        self.txactions = { }
        self.parscode = ""
        self.tagnames = { }
        self.attrnames = { }
        self.classname = "TXmlParser"
        self.usexmlns = False
        self.useholder = False
        self.processEmpty = False
        self.localpath = ""

    def enter(self, tree):
        if tree.nodeType == tree.ELEMENT_NODE:
            if tree.nodeName == "parse:include":
                return self.processInclude(tree, isRecursion = True)
            if tree.nodeName == "parse:empty_on":
                self.processEmpty = True
            if tree.nodeName == "parse:empty_off":
                self.processEmpty = False
            subitems = [ ]
            attrs = [ ]
            onenter = None;
            onleave = None;
            ontext = ""
            n = "tag_" + nm(tree.nodeName) + "_" + ("%04i" % self.elem)
            self.elem = self.elem + 1
            dupcheck = { }
            for subtree in tree.childNodes:
                if subtree.nodeType == subtree.ELEMENT_NODE:
                    if subtree.nodeName in dupcheck:
                        raise Exception("duplicate node '%s'" % subtree.nodeName)
                    if subtree.prefix == "parse":
                        value = ""
                        for textitem in subtree.childNodes:
                            if textitem.nodeType == textitem.TEXT_NODE:
                                value = value + textitem.nodeValue.strip()
                        if subtree.nodeName == "parse:enter":
                            onenter = "OnEnter_" + n
                            self.elactions[onenter] = value
                            dupcheck[subtree.nodeName] = True
                        elif subtree.nodeName == "parse:leave":
                            onleave = "OnLeave_" + n
                            self.elactions[onleave] = value
                            dupcheck[subtree.nodeName] = True
                        elif subtree.nodeName == "parse:include":
                            subitems.extend(self.enter(subtree))
                        elif subtree.nodeName == "parse:empty_on":
                            self.processEmpty = True
                        elif subtree.nodeName == "parse:empty_off":
                            self.processEmpty = False
                        else:
                            raise Exception("unknown action " + subtree.nodeName)
                    else:
                        dupcheck[subtree.nodeName] = True
                        subitems.append(self.enter(subtree))
                elif subtree.nodeType == subtree.TEXT_NODE:
                    if subtree.nodeValue.strip() != "":
                        ontext = ontext + subtree.nodeValue.strip()
            s = ""
            if tree.hasAttributes():
                for i in xrange(tree.attributes.length):
                    att = tree.attributes.item(i)
                    if "xmlns:" in att.nodeName:
                        continue
                    self.attrnames[att.nodeName] = True
                    a = n + "_attr_" + nm(att.nodeName);
                    s = s + tab*3 + nm(a) + " = " + "attr_" + nm(att.nodeName)
                    if att.nodeValue is not None and att.nodeValue.strip() != "":
                        self.atactions[a] = att.nodeValue.strip()
                        s = s + " >{ OnAttr_" + nm(a) + "(dataptr); }"
                    attrs.append(a)
                    s = s + ";\n"
            if ontext != "":
                s = s + tab*3 + n + "_text = text >{ OnText_" + n + "(dataptr, datalen); };\n"
                subitems.append(n + "_text")
                self.txactions[n] = ontext
            self.tagnames[tree.nodeName] = True
            s = s + tab*3 + n + " = enter_" + nm(tree.nodeName)
            if onenter is not None:
                s = s + " >{ " + onenter + "(); }"
            if len(attrs) > 0:
                s = s + " (" + " | ".join([nm(x) for x in attrs]) + ")*"
            if len(subitems) > 0:
                s = s + " (" + " | ".join(subitems) + ")*"
            else:
                s = s + " text*"
            s = s + " leave_" + nm(tree.nodeName)
            if ontext != "":
                s = s + " @{ "
                if not self.processEmpty:
                    s = s + "if (! FirstText)"
                s = s + " OnText_" + n + "(NULL, 0); }"
            if onleave is not None:
                s = s + " @{ " + onleave + "(); }";
            s = s + ";\n";
            self.parscode = self.parscode + s
            return n
        else:
            raise Exception("bad node type of " + tree.nodeType);

    def assignlexems(self):
        self.tags = { }
        i = 1
        tagnames = self.tagnames.keys()
        tagnames.sort()
        for tag in tagnames:
            self.tags[tag] = i
            i = i + 1
        self.closedelta = len(tagnames)
        i = i + self.closedelta
        self.attrs = { }
        attrnames = self.attrnames.keys()
        attrnames.sort()
        for attr in attrnames:
             self.attrs[attr] = i
             i = i + 1

    def mklexpartcode(self, what):
        strs = [ ]
        for key, value in what.iteritems():
            strs.append("\"" + key + ("\" 0 @{ return %i; }" % value))
        return tab*3 + "main := " + (" |\n" + tab*5).join(strs) + ";\n"

    def mklexpart(self):
        return self.mklexpartcode(self.tags)

    def mkalexpart(self):
        return self.mklexpartcode(self.attrs)

    def mkparspart(self):
        s = tab*3 + "text = 0;\n"
        for key, value in self.tags.iteritems():
            s = s + tab*3 + "enter_" + nm(key) + (" = %i;\n" % value)
            s = s + tab*3 + "leave_" + nm(key) + (" = %i;\n" % (value + self.closedelta))
        for key, value in self.attrs.iteritems():
            s = s + tab*3 + "attr_" + nm(key) + (" = %i;\n" % value)
        return s + self.parscode;

    def mkcpppart(self):
        s = ""
        for name, content in self.elactions.iteritems():
            s = s + tab + "void " + name + "() {\n" + tab*2 + content + "\n" + tab + "}\n\n";
        for name, content in self.atactions.iteritems():
            if self.useholder:

                holderValue = tab*2 + "if (Holder.OnAttr(value)) {\n"
                holderValue += tab*3 + "OnAttr_" + name + "(Holder.GetResult());\n"
                holderValue += tab*3 + "Holder.Reset();\n"
                holderValue += tab*2 + "}\n"

                s = s + tab + "void OnAttr_" + name + "(const char* value) {\n" + tab*2 + holderValue + "\n" + tab + "}\n\n";
                s = s + tab + "void OnAttr_" + name + "(typename TXmlHolder::TStringParam value) {\n" + tab*2 + content + "\n" + tab + "}\n\n";
            else:
                s = s + tab + "void OnAttr_" + name + "(const char* value) {\n" + tab*2 + content + "\n" + tab + "}\n\n";
        for name, content in self.txactions.iteritems():
            if self.useholder:

                holderValue = tab*2 + "if (Holder.OnText(text, len)) {\n"
                holderValue += tab*3 + "OnText_" + name + "(Holder.GetResult());\n"
                holderValue += tab*3 + "Holder.Reset();\n"
                holderValue += tab*2 + "}\n"

                s = s + tab + "void OnText_" + name + "(const char* text, size_t len) {\n"
                s = s + holderValue
                s = s + tab + "}\n\n";
                s = s + tab + "void OnText_" + name + "(typename TXmlHolder::TStringParam value) {\n"
                s = s + tab*2 + "Y_UNUSED(value);\n";
                s = s + tab*2 + content + "\n" + tab + "}\n\n";
            else:
                s = s + tab + "void OnText_" + name + "(const char* text, size_t len) {\n"
                s = s + tab*2 + "Y_UNUSED(text);\n";
                s = s + tab*2 + "Y_UNUSED(len);\n";
                s = s + tab*2 + content + "\n" + tab + "}\n\n";
        return s


    def process(self, input):
        from hashlib import md5
        self.randomcookie = md5(input).hexdigest()
        self.processInclude(input)
        self.assignlexems()


    def processInclude(self, input, isRecursion = False):
        def parseAttrs(self, attrs):
            for i in xrange(attrs.length):
                att = attrs.item(i)
                if att.nodeName == "class":
                    self.classname = att.nodeValue
                if att.nodeName == "useholder":
                    self.useholder = att.nodeValue.lower() == "true" or att.nodeValue.lower() == "yes"

        def GetPath(node):
            for i in xrange(node.attributes.length):
                att = node.attributes.item(i)
                if att.nodeName == "path":
                    return self.localpath + att.nodeValue
            raise Exception("no path specified!!")

        from xml.dom.minidom import parse, Element

        if isinstance(input, Element):
            path = GetPath(input)
            input = open(path)

        tree = parse(input)
        tree.normalize()
        nodes = [ ]
        for subtree in tree.childNodes:
            if subtree.nodeType == subtree.ELEMENT_NODE and subtree.nodeName == "PARSER":
                if not isRecursion:
                    nodes = [ ]
                    if subtree.hasAttributes():
                        parseAttrs(self, subtree.attributes)

                for node in subtree.childNodes:
                    if node.nodeType == node.ELEMENT_NODE:
                        data = self.enter(node)
                        if isinstance(data, list):
                            nodes.extend(data)
                        else:
                            nodes.append(data)

                if not isRecursion:
                    s = tab*3 + "main := (" + " | ".join(nodes) + ")*;\n";
                    self.parscode = self.parscode + s
        return nodes


    def substitute(self, input):
        for line in file(input):
            if line.strip() == "%%% LEXER %%%":
                print self.mklexpart(),
            elif line.strip() == "%%% ATTRLEXER %%%":
                print self.mkalexpart(),
            elif line.strip() == "%%% PARSER %%%":
                print self.mkparspart(),
            elif line.strip() == "%%% CODE %%%":
                print self.mkcpppart(),
            else:
                if (line.find("%%ClassName%%") != -1):
                    line = line.replace("%%ClassName%%", self.classname)
                if (line.find("%%CloseDelta%%") != -1):
                    line = line.replace("%%CloseDelta%%", "%i" % self.closedelta)
                if (line.find("%%RandomCookie%%") != -1):
                    line = line.replace("%%RandomCookie%%", self.randomcookie)
                if (line.find("%%HolderValueClass%%") != -1):
                    if self.useholder:
                        line = line.replace("%%HolderValueClass%%", "")
                    else:
                        line = ""
                print line,

    def setusexmlns(self, value):
        self.usexmlns = value == "use"

def main(argv):
    comp = compiler()
    if len(argv) > 3 :
        comp.setusexmlns(argv[3])
    comp.localpath = argv[1][:argv[1].rfind('/') + 1]
    comp.process(argv[1])
    comp.substitute(argv[2])

if __name__ == "__main__":
    from sys import argv
    main(argv)
