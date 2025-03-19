#include "index.hpp"
#include <yandex/logger.h>
#include <chrono>

using namespace DPS::Writer;
using namespace std;

namespace DPS {
    namespace Handle {

        Index::Index(fastcgi::Request *request, fastcgi::HandlerContext *context) {

            forceRemoved = false;
            properties = true;
            fetchDirs = true;

            dir = request->getArg("dir");
            file = request->getArg("file");

            if (file.empty() && dir.empty()) {
                auto name = request->getArg("name");
                if (name.back() == '/') {
                    dir = name;
                } else {
                    file = name;
                }
            }

            if (dir.back() == '/')
                dir = dir.substr(0, dir.size() - 1);

            version = request->getArg("version");
            if (version.empty())
                version = "stable";

            forceRemoved = !request->getArg("removed").empty();
            fetchContent = !request->getArg("content").empty();
            recursive = !request->getArg("recursive").empty();

            format = request->getArg("format");
            transform(format.begin(), format.end(), format.begin(), ::tolower);

            auto propsArg = request->getArg("properties");
            transform(propsArg.begin(), propsArg.end(), propsArg.begin(), ::tolower);

            if (propsArg == "false" || propsArg == "no" || propsArg == "0")
                properties = false;

            auto dirsArg = request->getArg("directories");
            transform(dirsArg.begin(), dirsArg.end(), dirsArg.begin(), ::tolower);

            if (dirsArg == "false" || dirsArg == "no" || dirsArg == "0")
                fetchDirs = false;

            req = request;
            ctx = context;

            process();
        };

        void Index::process() {
            MysqlLayer* mysql = &SingletonMysqlLayer::Instance();

            if (dir.empty() && file.empty()) {
                req->setStatus(400);
                fastcgi::RequestStream stream(req);
                stream << "'file' or 'dir' parameters are required";                
                return;
            }

            if (!dir.empty() && !file.empty()) {
                req->setStatus(400);
                fastcgi::RequestStream stream(req);
                stream << "'file' or 'dir' parameters are required, not both";                
                return;
            }

            req->setStatus(200);

            if (!file.empty()) {
                req->setHeader("X-DPS-Entry-Type", "file");
                Entry entry = mysql->ReadFile(file, version, forceRemoved);
                fastcgi::RequestStream stream(req);
                stream << entry.content;	
                
            } else {
                req->setHeader("X-DPS-Entry-Type", "dir");
                processDirectory();		
            }  
        }	

        void Index::processDirectory() {
            MysqlLayer* mysql = &SingletonMysqlLayer::Instance();

            Entries entries = mysql->ReadDir(dir, version, recursive, fetchContent, properties, fetchDirs, forceRemoved);
            fastcgi::RequestStream stream(req);
            shared_ptr<AbstractWriter> writer;

            Entry* root = NULL;
            for (Entry entry : entries) {
                if (entry.path == dir) {
                    root = &entry;
                    break;
                }
            }

            if (root == NULL) {
                req->setStatus(404);
                stream << "Error: Not found " << dir;
                return;
            }


            if (format == "json") {
                req->setContentType("application/json; charset=utf-8");
                writer = shared_ptr<AbstractWriter>(new JSONWriter(&stream));
            } else if (format == "msgpack") {
                req->setContentType("application/octet-stream");
                writer = shared_ptr<AbstractWriter>(new MsgPackWriter(&stream, entries.size()));
            } else {
                req->setContentType("text/xml; charset=utf-8");
                writer = shared_ptr<AbstractWriter>(new XMLWriter(&stream, *root));
            }

            for (Entry entry : entries) {
                if (entry.path != dir) {
                    writer->writeItem(entry, fetchContent);	
                }
            }
        }
    }
}
