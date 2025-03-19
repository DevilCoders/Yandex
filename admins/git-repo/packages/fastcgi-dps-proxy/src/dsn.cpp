#include <boost/lexical_cast.hpp>
#include <yandex/string_helpers.h>
#include <dbpool/dsn.h>
#include <dbpool/handle_var.h>

namespace mypp
{

void
ParseString(DSN& dsn, const std::string& str)
{
	DataVector p = Split(str, ",");

	dsn.port = 0;
	for(DataVector::iterator i=p.begin(); i!=p.end(); ++i){
		std::pair<std::string, std::string> pair = BaseName(*i, "=");
		if(pair.first == "driver")
			dsn.driver = pair.second;
		else if(pair.first == "host")
			dsn.host = pair.second;
		else if(pair.first == "user")
			dsn.user = pair.second;
		else if(pair.first == "password")
			dsn.passwd = pair.second;
		else if(pair.first == "name")
			dsn.db = pair.second;
		else if(pair.first == "port"){
			try{
				dsn.port = boost::lexical_cast<int>(pair.second);
			}
			catch(...){
				throw std::runtime_error("wrong dsn");
			}
		}
	}
}
	
struct MakeDSN
{
	DSN_list& rv;
	MakeDSN(DSN_list& r) : rv(r) {};

	void operator()(const std::string& s) {
		DSN dsn;
		ParseString(dsn, s);

		rv.push_back(dsn);
	};
};
	
DSN_list CreateFromString(const std::string& s)
{
	DSN_list rv;

	DataVector parts = Split(s, ";");
	std::for_each(parts.begin(), parts.end(), MakeDSN(rv));

	LOG(LogLevel::DEBUG, "Created " << rv.size() << " dsn list\n");
	
	return rv;
};

};
