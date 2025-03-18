#include "snippet_dump_xml_writer.h"
#include <kernel/snippets/util/xml.h>
#include <libxml/encoding.h>
#include <util/string/builder.h>

namespace NSnippets
{
    static xmlChar* Convert(const char* in, const char* encoding = "UTF-8")
    {
        xmlChar *out;
        int ret;
        int size;
        int out_size;
        int temp;
        xmlCharEncodingHandlerPtr handler;

        if (in == nullptr)
            return nullptr;

        handler = xmlFindCharEncodingHandler(encoding);

        if (!handler) {
            Cerr << "convert: no encoding handler found for '" << ( encoding ? encoding : "") << "'" << Endl;
            return nullptr;
        }

        size = (int) strlen(in) + 1;
        out_size = size * 2 - 1;
        out = (unsigned char *) xmlMalloc((size_t) out_size);

        if (out != nullptr) {
            temp = size - 1;
            ret = handler->input(out, &out_size, (const xmlChar *) in, &temp);
            if ((ret < 0) || (temp - size + 1)) {
                if (ret < 0) {
                    Cerr << "convert: conversion wasn't successful." << Endl;
                } else {
                        Cerr << "convert: conversion wasn't successful. converted: " << temp << " octets." << Endl;
                }

                xmlFree(out);
                out = nullptr;
            } else {
                out = (unsigned char *) xmlRealloc(out, out_size + 1);
                out[out_size] = 0;  /*null terminating out */
            }
        } else {
            Cerr << "convert: no mem" << Endl;
        }

        return out;
    }

    static void WriteElement(xmlTextWriterPtr writer, const char* name, const char* data)
    {
        xmlChar* tn = Convert(name);
        xmlChar* td = Convert(data);

        int rc = xmlTextWriterStartElement(writer, tn);
        Y_UNUSED(rc);
        assert(0 <= rc);

        rc = xmlTextWriterWriteCDATA(writer, td);
        assert(0 <= rc);

        rc = xmlTextWriterEndElement(writer);
        assert(0 <= rc);

        rc = xmlTextWriterWriteString(writer, BAD_CAST "\n");
        assert(0 <= rc);

        if (tn != nullptr)
            xmlFree(tn);
        if (td != nullptr)
            xmlFree(td);
    }

    static TUtf16String XmlFixup(const TUtf16String& s)
    {
        TUtf16String res = s;
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == TChar(0xFFFF))
            {
                res[i] = ' ';
            }
        }
        return res;
    }

    void WriteTextElement(xmlTextWriterPtr writer, const TString& name, const TString& value)
    {
        xmlChar* attrName = Convert(name.data());
        xmlChar* attrValue = Convert(EncodeTextForXml10(value, false).data());

        // <name>
        xmlTextWriterStartElement(writer, attrName);

        xmlTextWriterWriteString(writer, BAD_CAST "\n");
        xmlTextWriterWriteString(writer, attrValue);
        xmlTextWriterWriteString(writer, BAD_CAST "\n");

        // </name>
        xmlTextWriterEndElement(writer);

        if (attrName != nullptr)
            xmlFree(attrName);
        if (attrValue != nullptr)
            xmlFree(attrValue);
    }

    void WriteAttributeIntoElement(xmlTextWriterPtr writer, const TString& name, const TString& value)
    {
        if (value.empty()) {
            return;
        }

        xmlChar* attrName = Convert(name.data());
        xmlChar* attrValue = Convert(EncodeTextForXml10(value, false).data());

        xmlTextWriterWriteAttribute(writer, attrName, attrValue);
        if (attrName != nullptr) {
            xmlFree(attrName);
        }

        if (attrValue != nullptr) {
            xmlFree(attrValue);
        }
    }

    // *** TSnippetDumpXmlWriter ***

    void TSnippetDumpXmlWriter::Start()
    {
        Out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<pools>" << Endl;
    }

    void TSnippetDumpXmlWriter::StartPool(const TString& name)
    {
        Out << Sprintf("<pool name=\"%s\">", name.data()).data() << Endl;
    }

    void TSnippetDumpXmlWriter::FinishPool()
    {
        Out << "\n</pool>" << Endl;
    }

    void TSnippetDumpXmlWriter::Finish()
    {
        Out << Endl << "</pools>" << Endl;
    }

    void TSnippetDumpXmlWriter::WriteFeatures(xmlTextWriterPtr writer, const TString& featuresString) {
        // <features>

        xmlTextWriterWriteString(writer, BAD_CAST "\n");
        WriteTextElement(writer, "features", featuresString.data());
        xmlTextWriterWriteString(writer, BAD_CAST "\n");

        // </features>
    }

    void TSnippetDumpXmlWriter::WriteFragment(xmlTextWriterPtr writer, const TSnipFragment& fragment)
    {
        xmlTextWriterWriteString(writer, BAD_CAST "\n");

        // <fragment>
        xmlTextWriterStartElement(writer, BAD_CAST "fragment");

        WriteAttributeIntoElement(writer, "coords", fragment.Coords.data());
        WriteAttributeIntoElement(writer, "arccoords", fragment.ArcCoords.data());

        xmlChar* xmlText = Convert(EncodeTextForXml10(WideToUTF8(fragment.Text), false).data());
        xmlTextWriterWriteCDATA(writer, xmlText);

        // </fragment>
        xmlTextWriterEndElement(writer);

        if (xmlText != nullptr) {
            xmlFree(xmlText);
        }
    }

    void TSnippetDumpXmlWriter::WriteMark(xmlTextWriterPtr writer, const TSnipMark& mark)
    {
        xmlTextWriterWriteString(writer, BAD_CAST "\n");
        // <mark>
        xmlTextWriterStartElement(writer, BAD_CAST "mark");

        WriteAttributeIntoElement(writer, "value", Sprintf("%d", mark.Value).data());
        WriteAttributeIntoElement(writer, "criteria", WideToUTF8(mark.Criteria).data());
        WriteAttributeIntoElement(writer, "assessor", WideToUTF8(mark.Assessor).data());
        WriteAttributeIntoElement(writer, "quality", Sprintf("%f", mark.Quality).data());
        WriteAttributeIntoElement(writer, "timestamp", WideToUTF8(mark.Timestamp).data());

        // </mark>
        xmlTextWriterEndElement(writer);
    }

    void TSnippetDumpXmlWriter::WriteSnippet(xmlTextWriterPtr writer, const TReqSnip& snippet)
    {
        xmlTextWriterWriteString(writer, BAD_CAST "\n");

        // <snippet>
        xmlTextWriterStartElement(writer, BAD_CAST "snippet");

        WriteAttributeIntoElement(writer, "algorithm", snippet.Algo.data());
        WriteAttributeIntoElement(writer, "fragments", (TStringBuilder() << "" << snippet.SnipText.size()).data());
        WriteAttributeIntoElement(writer, "rank", snippet.Rank.data());

        //snippet lines number for steam
        if (!snippet.Lines.empty())
            WriteAttributeIntoElement(writer, "lines", snippet.Lines.data());

        WriteElement(writer, "title", EncodeTextForXml10(WideToUTF8(XmlFixup(snippet.TitleText)).data(), false).data());

        for (TVector<TSnipFragment>::const_iterator fragmentIter = snippet.SnipText.begin(); fragmentIter != snippet.SnipText.end(); ++fragmentIter) {
            WriteFragment(writer, *fragmentIter);
        }

        // <features>
        if (!snippet.FeatureString.empty()) {
            WriteFeatures(writer, snippet.FeatureString);
        }
        // </features>

        if (snippet.Marks.size() > 0) {
            // <marks>
            xmlTextWriterStartElement(writer, BAD_CAST "marks");
            for (TVector<TSnipMark>::const_iterator markIter = snippet.Marks.begin(); markIter != snippet.Marks.end(); ++markIter) {
                WriteMark(writer, *markIter);
            }
            // </marks>
            xmlTextWriterEndElement(writer);
        }

        // </snippet>
        xmlTextWriterEndElement(writer);
    }

    // ---- Fast API methods for writing snippet consequentially
    void TSnippetDumpXmlWriter::StartQDPair(const TString& query, const TString& url, const TString& relevance,
                                            const TString& region, const TString& richTreee)
    {
        xmlTextWriterPtr writer;
        xmlBufferPtr buf;

        buf = xmlBufferCreate();
        writer = xmlNewTextWriterMemory(buf, 0);

        // <qdpair>
        xmlTextWriterStartElement(writer, BAD_CAST "qdpair");

        WriteAttributeIntoElement(writer, "region", region);
        WriteAttributeIntoElement(writer, "richtree", richTreee);
        WriteAttributeIntoElement(writer, "url", url);
        WriteAttributeIntoElement(writer, "relevance", relevance);

        // query
        xmlChar* xmlText = Convert(EncodeTextForXml10(query, false).data());
        xmlTextWriterWriteCDATA(writer, xmlText);

        if (xmlText != nullptr) {
            xmlFree(xmlText);
        }

        xmlFreeTextWriter(writer);
        Out << (const char*)buf->content;
        xmlBufferFree(buf);
    }

    void TSnippetDumpXmlWriter::WriteSnippet(const TReqSnip& snippet)
    {
        xmlTextWriterPtr writer;
        xmlBufferPtr buf;

        buf = xmlBufferCreate();
        writer = xmlNewTextWriterMemory(buf, 0);

        WriteSnippet(writer, snippet);

        xmlFreeTextWriter(writer);
        Out << (const char*)buf->content;
        xmlBufferFree(buf);
    }

    void TSnippetDumpXmlWriter::FinishQDPair()
    {
        Out << "\n</qdpair>" << Endl;
    }
}
