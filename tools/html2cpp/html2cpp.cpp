#include <cstring>
#include <cerrno>
#include <iostream>
#include <fstream>

using namespace std;

const char* beginCharOut = "    YxPrint(ysc, \"";   //18
const char* endCharOut = "\\n\");";                 //5
const char* endCharOut2 = "\");";                   //3
const char* beginValueOut = "    YxPrint(ysc, ";    //17
const char* endValueOut = ");";                     //2
const char* endline = "\n";                         //1

template <typename T>
T getnext(T p)
{
    T t = strpbrk(p, "\n\'\"\\<%");
    if(t == nullptr)
        return nullptr;
    else if(*t == '<' && *(t+1) != '%')
        return getnext(t+1);
    else if(*t == '%' && *(t+1) != '>')
        return getnext(t+1);
    else
        return t;
}

class Recoder
{
private:
    char buf[1024];
    int status;
    int nextstatus;
    char* curPos;
    char* GetChunk(char*& p);
public:
    Recoder()
        : status(0)
        , nextstatus(0)
        , curPos(buf)
    {}
    void Encode(fstream& os, fstream& is);
};

char* Recoder::GetChunk(char*& p)
{
    status = nextstatus;
    char* t = getnext(p);
    if(t == nullptr)
    {
        memcpy(curPos,p,strlen(p));
        curPos += strlen(p);
        return nullptr;
    }
    int n;
    char* next;
    bool stop;
    if(*t == '<')
    {
        n = t-p;
        stop = true;
        next = t+2;
        if(*next == '=')
        {
            next += 1;
            nextstatus = 2;
        }
        else
            nextstatus = 1;
    }
    else if(*t == '%')
    {
        n = t-p;
        stop = true;
        next = t + 2;
        nextstatus = 0;
    }
    else if(*t == '\n')
    {
        stop = true;
        next = t + 1;
        n = t-p;
    }
    else
    {
        stop = false;
        next = t + 1;
        n = t-p;
    }
    memcpy(curPos,p,n);
    curPos += n;
    if(stop == false)
    {
        if(status == 0)
            *curPos++ = '\\';
        *curPos++ = *t;
    }
    p = next;
    if(stop)
    {
        *curPos = '\0';
        curPos = buf;
        return buf;
    }
    return GetChunk(p);
}

void crlf2lf(char *c) {
    char *d;
    for(d = c; *c; c++) {
        if (*c != '\r' || c[1] != '\n')
            *d++ = *c;
    }
    *d = 0;
}

void Recoder::Encode(fstream& os, fstream& is)
{
    is.seekg(0, ios::end);
    int count = is.tellg();
    char* ci = new char[count+1];
    is.seekg(0, ios::beg);
    is.read(ci, count);
    ci[count]='\0';
#if defined(_WIN32) || defined(__CYGWIN__) // ...magic...
    crlf2lf(ci);
#endif
    char* p = ci;
    while(*p != '\0')
    {
        char* ch = GetChunk(p);
        if(ch == nullptr)
            break;
        if(*ch == '\0')
            continue;
        if(status == 0)
            os.write(beginCharOut,18);
        else if(status == 2)
            os.write(beginValueOut,17);
        os.write(ch,strlen(ch));
        if(status == 0)
        {
            if(nextstatus != 2)
                os.write(endCharOut,5);
            else
                os.write(endCharOut2,3);
        }
        else if(status == 2)
            os.write(endValueOut,2);
        os.write(endline,1);
    }
}

int main(int argc, char* argv[])
{
    if(argc < 3) {
        cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << endl;
        return 1;
    }
    fstream is, os;
    is.open(argv[1],ios::in|ios::binary);
    if(!is.is_open()) {
        cerr << "Error: can't open file " << argv[1] << ": " << strerror(errno) << endl;
        return 1;
    }
    os.open(argv[2],ios::out|ios::trunc|ios::binary);
    if(!os.is_open()) {
        cerr << "Error: can't open file " << argv[2] << ": " << strerror(errno)  << endl;
        return 1;
    }
    Recoder Rc;
    Rc.Encode(os,is);
    is.close();
    os.close();
    return 0;
}
