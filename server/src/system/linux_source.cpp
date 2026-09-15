#include "server/system/linux_source.hpp"
#include "common/unique_fd.hpp"
#include <array>
#include <charconv>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

namespace web_htop::server::system
{
namespace
{
FileResult Read(std::string const& path)
{
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK));

    if (!fd)
    {
        return {{}, errno};
    }
    FileResult result;
    std::array<char, 8192> buffer{};

    for (;;)
    {
        auto n = ::read(fd.Get(), buffer.data(), buffer.size());

        if (n == 0)
        {
            return result;
        }
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return {{}, errno};
        }
        if (result.text.size() + static_cast<std::size_t>(n) > 8 * 1024 * 1024)
        {
            return {{}, EFBIG};
        }
        result.text.append(buffer.data(), static_cast<std::size_t>(n));
    }
}
} // namespace

FileResult LinuxSource::Proc(std::string const& relative) const
{
    return Read(config_.proc_root + "/" + relative);
}

FileResult LinuxSource::Sys(std::string const& relative) const
{
    return Read(config_.sys_root + "/" + relative);
}

FileResult LinuxSource::Cgroup(std::string const& relative) const
{
    return Read(config_.cgroup_path + "/" + relative);
}

PidList LinuxSource::Pids() const
{
    PidList result;
    std::error_code ec;
    std::filesystem::directory_iterator it(config_.proc_root, ec), end;

    if (ec)
    {
        return {{}, ec.value()};
    }
    for (; it != end; it.increment(ec))
    {
        if (ec)
        {
            result.error = ec.value();
            break;
        }
        auto name = it->path().filename().string();
        int pid{};
        auto [last, error] = std::from_chars(name.data(), name.data() + name.size(), pid);

        if (error == std::errc{} && last == name.data() + name.size() && pid > 0)
        {
            result.pids.push_back(pid);
        }
    }
    if (ec)
    {
        result.error = ec.value();
    }
    return result;
}

int LinuxSource::Filesystem(struct statvfs& result) const
{
    return ::statvfs(config_.mount_path.c_str(), &result) == 0 ? 0 : errno;
}

std::string LinuxSource::CgroupIdentity() const
{
    struct stat st{};

    if (::stat(config_.cgroup_path.c_str(), &st) != 0)
    {
        return {};
    }
    return std::to_string(st.st_dev) + ":" + std::to_string(st.st_ino);
}
} // namespace web_htop::server::system
