/**
 * @file common/enums.hpp
 *
 * @author Maksim Vashkevich
 * @date 2026-04-03
 *
 * @brief Common enumerations for web_htop project
 * @details This header defines common enums used across the web_htop project,
 * such as process states and other.
 */

#ifndef WEB_HTOP_COMMON_ENUMS_HPP_
#define WEB_HTOP_COMMON_ENUMS_HPP_

namespace web_htop {

/**
 * @brief Process state enumeration based on /proc/[pid]/stat field 3.
 */
enum class ProcessState : char {
    RUNNING      = 'R', // Running
    SLEEPING     = 'S', // Sleeping in interruptible wait
    DISK_SLEEP   = 'D', // Waiting in uninterruptible disk sleep
    ZOMBIE       = 'Z', // Zombie
    STOPPED      = 'T', // Stopped (on a signal)
    TRACING_STOP = 't', // Tracing stop
    DEAD         = 'X', // Dead
    WAKING       = 'W', // Waking
    PARKED       = 'P', // Parked
    IDLE         = 'I', // Idle (Linux 5.6+)
    UNKNOWN      = '?'  // Unknown state
};

} // namespace web_htop

#endif // WEB_HTOP_COMMON_ENUMS_HPP_