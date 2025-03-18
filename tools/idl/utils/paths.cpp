#include <yandex/maps/idl/utils/paths.h>

#include <yandex/maps/idl/utils/common.h>
#include <yandex/maps/idl/utils/errors.h>
#include <yandex/maps/idl/utils/exception.h>

#include <filesystem>
#include <fstream>

namespace yandex::maps::idl::utils {

Path Path::withExtension(std::string extension) const
{
    return Value_(value_).replace_extension(extension);
}

Path Path::withSuffix(std::string suffix) const
{
    return Path(asString("", suffix));
}

Path Path::capitalized() const
{
    std::string capitalized;
    for (const auto& part : value_) {
        if (!capitalized.empty()) {
            capitalized += '/';
        }
        capitalized += capitalizeWord(part.generic_string());
    }

    return Path(capitalized);
}

Path Path::camelCased(bool capitalizeFirstSymbol) const
{
    std::string camelCased;
    for (const auto& part : value_) {
        if (!camelCased.empty()) {
            camelCased += '/';
        }
        camelCased +=
            toCamelCase(part.generic_string(), capitalizeFirstSymbol);
    }
    return Path(camelCased);
}

void Path::remove() const
{
    std::filesystem::remove_all(value_);
}

void Path::write(const std::string& contents) const
{
    auto dirPath = parent();
    if (std::filesystem::exists(dirPath)) {
        if (!std::filesystem::is_directory(dirPath)) {
            throw UsageError() << "Not a directory: " << dirPath.inQuotes();
        }
    } else if (!std::filesystem::create_directories(dirPath)) {
        throw UsageError() <<
            "Could not create directory " << parent().inQuotes();
    }

    std::ofstream stream(asString(), std::ios_base::out | std::ios_base::binary);
    if (!stream) {
        throw UsageError() << "Could not write to file " << inQuotes();
    }

    stream << contents;
}

std::string Path::read() const
{
    std::ifstream in(asString());
    if (!in) {
        throw UsageError() << "Could not read file " << inQuotes();
    }

    std::stringstream buffer;
    buffer << in.rdbuf();

    return buffer.str();
}

std::vector<Path> Path::childPaths(
    bool makeRelative,
    std::string extension) const
{
    std::vector<utils::Path> childPaths;

    std::filesystem::recursive_directory_iterator iterator(value_);
    std::filesystem::recursive_directory_iterator end;
    for (; iterator != end; ++iterator) {
        if (!extension.empty()) {
            if (!std::filesystem::is_regular_file(iterator->path()) ||
                    iterator->path().extension() != extension) {
                continue;
            }
        }

        auto childPath = iterator->path();
        if (makeRelative) {
            childPath = childPath.generic_string().substr(
                value_.generic_string().size() + 1);
        }

        childPaths.push_back(childPath);
    }

    return childPaths;
}

Path& Path::operator+=(const Path& other)
{
    value_ /= other.value_;
    return *this;
}

Path operator+(const Path& left, const Path& right)
{
    Path result(left);
    result += right;
    return result;
}

std::ostream& operator<<(std::ostream& out, const Path& path)
{
    return out << path.inQuotes();
}

namespace {

template <typename RawPaths>
std::vector<Path> from(RawPaths rawPaths)
{
    std::vector<Path> paths;
    paths.reserve(rawPaths.size());

    for (auto rawPath : rawPaths) {
        paths.emplace_back(rawPath);
    }

    return paths;
}

} // namespace

SearchPaths::SearchPaths(std::vector<std::string> rawPaths)
    : Wrapper(from(rawPaths))
{
}

SearchPaths::SearchPaths(std::initializer_list<const char*> rawPaths)
    : Wrapper(from(rawPaths))
{
}

std::size_t SearchPaths::resolve(const Path& relativePath) const
{
    for (std::size_t i = 0; i < value_.size(); ++i) {
        if (std::filesystem::is_regular_file(value_[i] + relativePath)) {
            return i;
        }
    }

    return std::size_t(-1);
}

} // namespace yandex::maps::idl::utils
