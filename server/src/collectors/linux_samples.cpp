#include "server/collectors/linux_samples.hpp"
#include <charconv>
#include <cmath>
#include <limits>
#include <numeric>

namespace web_htop::server::collectors
{
namespace
{
template <typename T> bool Parse(std::string_view s, T& value)
{
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    return ec == std::errc{} && p == s.data() + s.size();
}

constexpr std::string_view NextLine(std::string_view& s) noexcept
{
    if (s.empty())
    {
        return {};
    }
    const auto pos = s.find('\n');
    if (pos == std::string_view::npos)
    {
        auto line = s;
        s = {};
        if (!line.empty() && line.back() == '\r')
        {
            line.remove_suffix(1);
        }
        return line;
    }
    auto line = s.substr(0, pos);
    s.remove_prefix(pos + 1);
    if (!line.empty() && line.back() == '\r')
    {
        line.remove_suffix(1);
    }
    return line;
}

constexpr std::string_view NextToken(std::string_view& s) noexcept
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
    {
        s.remove_prefix(1);
    }
    if (s.empty())
    {
        return {};
    }
    const auto pos = s.find_first_of(" \t\r\n");
    if (pos == std::string_view::npos)
    {
        auto token = s;
        s = {};
        return token;
    }
    auto token = s.substr(0, pos);
    s.remove_prefix(pos);
    return token;
}
} // namespace

CpuSample ParseCpu(std::string_view text)
{
    CpuSample result;
    auto remaining = text;

    while (!remaining.empty())
    {
        auto line = NextLine(remaining);
        auto label = NextToken(line);

        if (!label.starts_with("cpu"))
        {
            continue;
        }
        int id = -1;

        if (label != "cpu" && (!Parse(label.substr(3), id) || id < 0))
        {
            continue;
        }
        CpuTimes times{};
        bool valid = true;

        for (auto& value : times)
        {
            auto field = NextToken(line);

            if (field.empty() || !Parse(field, value))
            {
                valid = false;
                break;
            }
        }
        if (valid)
        {
            result.emplace(id, times);
        }
    }
    return result;
}

std::optional<double> CpuUsage(CpuTimes const& now, CpuTimes const& previous)
{
    double total = 0, idle = 0;

    for (std::size_t i = 0; i < now.size(); ++i)
    {
        if (now[i] < previous[i])
        {
            return std::nullopt;
        }
        const double delta = static_cast<double>(now[i] - previous[i]);
        total += delta;

        if (i == 3 || i == 4)
        {
            idle += delta;
        }
    }
    if (total <= 0)
    {
        return std::nullopt;
    }
    return 100.0 * (total - idle) / total;
}

std::optional<double> CounterRate(std::uint64_t now, std::uint64_t previous, double seconds)
{
    if (now < previous || seconds <= 0 || !std::isfinite(seconds))
    {
        return std::nullopt;
    }
    return static_cast<double>(now - previous) / seconds;
}

std::optional<ProcessSample> ParseProcess(std::string_view text)
{
    const auto open = text.find('('), close = text.rfind(')');

    if (open == std::string_view::npos || close == std::string_view::npos || close <= open ||
        open < 2)
    {
        return std::nullopt;
    }
    ProcessSample s;
    auto pid = text.substr(0, open);

    while (!pid.empty() && pid.back() == ' ')
    {
        pid.remove_suffix(1);
    }
    if (!Parse(pid, s.pid) || s.pid <= 0)
    {
        return std::nullopt;
    }
    s.name = text.substr(open + 1, close - open - 1);
    auto tail = text.substr(close + 1);
    std::array<std::string_view, 22> fields{};

    for (auto& field : fields)
    {
        field = NextToken(tail);
        if (field.empty())
        {
            return std::nullopt;
        }
    }
    std::uint64_t user{}, system{};
    std::int64_t rss{};

    if (fields[0].size() != 1 || !Parse(fields[1], s.ppid) || !Parse(fields[11], user) ||
        !Parse(fields[12], system) || !Parse(fields[17], s.threads) ||
        !Parse(fields[19], s.starttime) || !Parse(fields[21], rss))
    {
        return std::nullopt;
    }
#if defined(__cpp_lib_saturation_arithmetic) && __cpp_lib_saturation_arithmetic >= 202311L
    s.ticks = std::add_sat(user, system);
    s.rss_pages = std::saturate_cast<std::uint64_t>(std::max<std::int64_t>(0, rss));
#else
    if (user > std::numeric_limits<std::uint64_t>::max() - system)
    {
        return std::nullopt;
    }
    s.ticks = user + system;
    s.rss_pages = rss > 0 ? static_cast<std::uint64_t>(rss) : 0;
#endif
    s.state = fields[0][0];
    return s;
}

models::PressureMetrics ParsePressure(std::string resource, std::string_view text)
{
    models::PressureMetrics result;
    result.resource = std::move(resource);
    auto remaining = text;

    while (!remaining.empty())
    {
        auto line = NextLine(remaining);
        auto type = NextToken(line);

        while (!line.empty())
        {
            auto token = NextToken(line);
            if (token.empty())
            {
                break;
            }
            auto equal = token.find('=');

            if (equal == std::string_view::npos)
            {
                continue;
            }
            auto key = token.substr(0, equal);
            double n{};

            if (!Parse(token.substr(equal + 1), n) || !std::isfinite(n) || n < 0 || n > 100)
            {
                continue;
            }
            if (type == "some" && key == "avg10")
            {
                result.some_avg10 = n;
            }
            if (type == "some" && key == "avg60")
            {
                result.some_avg60 = n;
            }
            if (type == "some" && key == "avg300")
            {
                result.some_avg300 = n;
            }
            if (type == "full" && key == "avg10")
            {
                result.full_avg10 = n;
            }
        }
    }
    return result;
}
} // namespace web_htop::server::collectors
