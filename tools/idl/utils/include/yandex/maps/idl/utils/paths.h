#pragma once

#include <yandex/maps/idl/utils/wrapper.h>

#include <cstddef>
#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

namespace yandex::maps::idl::utils {

/**
 * std::filesystem::path provides different ways to convert path to string,
 * which me we should be careful and call e.g. path.generic_string() instead
 * of path.string(). To avoid confusions like, all paths should be
 * represented by this simple class.
 */
class Path : public Wrapper<std::filesystem::path> {
public:
    using Wrapper::Wrapper;

    Path(std::string pathString) : Wrapper(pathString) { }
    Path(const char* pathChars) : Path(std::string(pathChars)) { }

    bool isEmpty() const { return value_.empty(); }

    std::string asString() const { return value_.generic_string(); }
    std::string asString(std::string prefix, std::string suffix = "") const
    {
        return prefix + asString() + suffix;
    }

    operator std::string() const { return asString(); }

    Path first() const { return Path(*value_.begin()); }
    Path last() const { return Path(value_.filename()); }

    std::string inQuotes() const { return asString("'", "'"); }
    std::string inAngles() const { return asString("<", ">"); }

    std::string extension() const { return Path(value_.extension()); }
    Path withExtension(std::string extension) const;
    Path withSuffix(std::string suffix) const;

    Path parent() const { return Path(value_.parent_path()); }
    Path fileName() const { return Path(value_.filename()); }
    Path stem() const { return Path(value_.stem()); }

    /**
     * Returns a copy of this path, but with all parts capitalized.
     */
    Path capitalized() const;

    /**
     * Returns a copy of this path, but with all parts camel-cased.
     */
    Path camelCased(bool capitalizeFirstSymbol = false) const;

    Path& operator+=(const Path& other);

    void remove() const;
    void write(const std::string& contents) const;
    std::string read() const;

    /**
     * Returns paths to all files & directories inside this path (which must
     * be a directory, of course).
     *
     * @param makeRelative - return paths relative to this one
     * @param extension - if not empty, returns only files, and with this
     *                    exact extension
     */
    std::vector<Path> childPaths(
        bool makeRelative = false,
        std::string extension = "") const;
};

Path operator+(const Path& left, const Path& right);

std::ostream& operator<<(std::ostream& out, const Path& path);

class SearchPaths : public Wrapper<std::vector<Path>> {
public:
    using Wrapper::Wrapper;

    SearchPaths(std::vector<std::string> rawPaths);
    SearchPaths(std::initializer_list<const char*> rawPaths);

    SearchPaths(Path path) : Wrapper({ path }) { }
    SearchPaths(const char* rawPath) : SearchPaths(Path(rawPath)) { }

    const Path& operator[](std::size_t i) const { return value_[i]; }
    Path& operator[](std::size_t i) { return value_[i]; }

    const Path& first() const { return value_.front(); }
    const Path& last() const { return value_.back(); }

    /**
     * @return index of search path that contains a file with combined path:
     *         search path + relative path. Returns std::size_t(-1) if no
     *         such search path was found.
     */
    std::size_t resolve(const Path& relativePath) const;
};

} // namespace yandex::maps::idl::utils

namespace std {

template <>
struct hash<yandex::maps::idl::utils::Path> {
    size_t operator()(const yandex::maps::idl::utils::Path& p) const
    {
        return hash<string>()(p);
    }
};

} // namespace std
