#ifndef FS_TYPES_H
#define FS_TYPES_H

#include <list>
#include <stdexcept>
#include <string>

using namespace std;

namespace DPS {

	class QueryException : public runtime_error {
		public:
			QueryException(const string& query, const string& arg1) 
				: runtime_error(
						query + "\n" +
						"Arguments: " + "\n" +
						"\t1: '" + arg1 + "'"
						) { };

			QueryException(const string& query, const string& arg1, const string& arg2) 
				: runtime_error(
						query + "\n" +
						"Arguments: " + "\n" +
						"\t1: '" + arg1 + "'\n"
						"\t2: '" + arg2 + "'"
						) { };

			QueryException(const string& query, const string& arg1, const string& arg2, const string& arg3) 
				: runtime_error(
						query + "\n" +
						"Arguments: " + "\n" +
						"\t1: '" + arg1 + "'\n"
						"\t2: '" + arg2 + "'\n"
						"\t3: '" + arg3 + "'"
						) { };

			QueryException(const string& query, const string& arg1, const string& arg2, const string& arg3, const string& arg4) 
				: runtime_error(
						query + "\n" +
						"Arguments: " + "\n" +
						"\t1: '" + arg1 + "'\n"
						"\t2: '" + arg2 + "'\n"
						"\t3: '" + arg3 + "'\n"
						"\t4: '" + arg4 + "'"
						) { };

			QueryException(const string& query) 
				: runtime_error(query) { };
	};	

	class NotFoundException : public std::runtime_error {
		public:
			NotFoundException(const string& path) 
				: std::runtime_error("Not found: " + path) { };
	};	

	typedef long int ID_t;

	struct Property {
		const string name;
		const string value;

		Property(const string& name, const string& value) : name(name), value(value) {};
	};

	typedef list<Property> PropList;
	typedef list<Property> Properties;

    struct Revision {
        const ID_t id;
        const string creationDate;
        Revision(const string& id, const string& cDate) : id(stol(id)), creationDate(cDate) {};
    };

    typedef list<Revision> Revisions;

	class Entry
	{
		public:
			enum Type {
				UNKNOWN,
				DIR,
				FILE
			};

			Entry(	const string& id, 
					const string& type, 
					const string& name, 
					const string& parentid, 
					const string& ctime, 
					const string& mtime, 
					const string& isDeleted, 
					const string& isLink, 
					const string& fullname, 
					const string& content, 
					const string& selectedversion,
					const string& stableversion ) :
				id(stol(id)),
				stype(type),
				type(type == "dir" ? Entry::DIR : Entry::FILE ),
				name(name), 
				parentid(stol(parentid)),
				creationDate(ctime), 
				modificationDate(mtime), 
				isDeleted(isDeleted != "no"), 
				isLink(isLink != "no"), 
				path(fullname), 
				content(content),
				selectedversion(stol(selectedversion)),
				stableversion(stol(stableversion)) {
			};

			bool IsDirectory() { return type == DIR; }
			bool IsFile() { return type == FILE; }

			const ID_t id;
			const string stype; 
			const Type type;
			const string name;
			const ID_t parentid;
			const string creationDate;
			const string modificationDate;
			const bool isDeleted;
			const bool isLink;
			const string path;
			const string content;
			const ID_t selectedversion;
			const ID_t stableversion;
			PropList properties;

	};

	typedef list<Entry> Entries;

}
#endif
