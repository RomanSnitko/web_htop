#include "client/app/client_app.hpp"
#include "client/net/stream_connection.hpp"
#include "client/ui/terminal_ui.hpp"
#include <csignal>
#include <fstream>
#include <iostream>
#include <poll.h>
#include <unistd.h>

namespace web_htop::client
{
namespace
{
volatile std::sig_atomic_t stopped = 0;

void OnSignal(int)
{
    stopped = 1;
}
} // namespace

int ClientApp::Run()
{
    stopped = 0;

    struct sigaction action{}, old_int{}, old_term{};

    action.sa_handler = OnSignal;
    ::sigemptyset(&action.sa_mask);
    ::sigaction(SIGINT, &action, &old_int);
    ::sigaction(SIGTERM, &action, &old_term);

    struct RestoreSignals
    {
        struct sigaction& old_int;
        struct sigaction& old_term;

        ~RestoreSignals()
        {
            ::sigaction(SIGINT, &old_int, nullptr);
            ::sigaction(SIGTERM, &old_term, nullptr);
        }
    } restore{old_int, old_term};

    std::unique_ptr<StreamConnection> stream;
    std::unique_ptr<HttpProbe> probe;
    // Resolve once before entering the terminal workspace. Connection/read deadlines
    // cover sockets; libc name resolution follows the machine's resolver policy.
    if (options_.replay_path.empty())
    {
        stream = std::make_unique<StreamConnection>(Resolve(options_.host, options_.stream_port));
        probe = std::make_unique<HttpProbe>(Resolve(options_.host, options_.http_port));
    }
    std::ofstream record;

    if (!options_.record_path.empty())
    {
        record.open(options_.record_path, std::ios::app);

        if (!record)
        {
            throw std::runtime_error("cannot open recording");
        }
    }
    std::ifstream replay;

    if (!options_.replay_path.empty())
    {
        replay.open(options_.replay_path);

        if (!replay)
        {
            throw std::runtime_error("cannot open replay");
        }
    }
    ui::TerminalUi terminal;
    ui::ViewState view;
    view.recording = record.is_open();
    std::optional<models::SystemSnapshot> current, visible;
    auto last = std::chrono::steady_clock::time_point{};
    auto next_render = std::chrono::steady_clock::now(), next_replay = next_render;
    std::string previous_filter;
    bool quit = false;
    auto receive = [&](models::SystemSnapshot snapshot)
    {
        if (options_.once && quit)
        {
            return;
        }
        auto now = std::chrono::steady_clock::now();

        if (current && current->telemetry.instance_id == snapshot.telemetry.instance_id)
        {
            if (snapshot.telemetry.sequence <= current->telemetry.sequence)
            {
                return;
            }
            view.sequence_gaps += snapshot.telemetry.sequence - current->telemetry.sequence - 1;
        }
        else
        {
            view.cpu_history.clear();
            view.memory_history.clear();
            view.rx_history.clear();
        }
        if (record.is_open())
        {
            record << protocol::Encode(snapshot) << '\n';
            record.flush();

            if (!record)
            {
                throw std::runtime_error("recording write failed");
            }
        }
        if (options_.once || !terminal.Interactive())
        {
            std::cout << protocol::Encode(snapshot) << '\n' << std::flush;
        }
        current = std::move(snapshot);
        last = now;

        if (!view.paused)
        {
            visible = current;
            view.cpu_history.push_back(current->cpu.total_usage_percent);
            view.memory_history.push_back(current->memory.used_percent);
            view.rx_history.push_back(current->network.rx_kbps * 1024);

            for (auto history : {&view.cpu_history, &view.memory_history, &view.rx_history})
            {
                if (history->size() > 120)
                {
                    history->pop_front();
                }
            }
        }
        if (options_.once)
        {
            quit = true;
        }
    };

    while (!stopped && !quit)
    {
        auto now = std::chrono::steady_clock::now();

        if (stream)
        {
            stream->Tick(now);
            probe->Tick();
        }
        if (replay.is_open() && now >= next_replay)
        {
            std::string line;

            if (std::getline(replay, line))
            {
                if (line.size() > protocol::kMaxPayload)
                {
                    throw std::runtime_error("replay frame too large");
                }
                std::string error;
                auto snapshot = protocol::Decode(line, error);

                if (!snapshot)
                {
                    throw std::runtime_error("replay: " + error);
                }
                receive(std::move(*snapshot));
                next_replay = now + std::chrono::milliseconds(250);
            }
            else
            {
                view.connection = "replay finished";
                replay.close();

                if (!terminal.Interactive())
                {
                    quit = true;
                }
            }
        }
        pollfd fds[3]{{terminal.Interactive() ? STDIN_FILENO : -1, POLLIN, 0},
                      {stream ? stream->Fd() : -1, stream ? stream->Events() : short(0), 0},
                      {probe ? probe->Fd() : -1, probe ? probe->Events() : short(0), 0}};
        int ready = ::poll(fds, 3, 50);

        if (ready < 0 && errno != EINTR)
        {
            throw std::runtime_error("poll failed");
        }
        if (stream && fds[1].revents)
        {
            stream->Handle(fds[1].revents, receive);
        }
        if (probe && fds[2].revents)
        {
            probe->Handle(fds[2].revents);
        }
        if (fds[0].revents & POLLIN)
        {
            char keys[64];
            auto n = ::read(STDIN_FILENO, keys, sizeof(keys));

            for (ssize_t i = 0; i < n; ++i)
            {
                char key = keys[i];

                if (view.editing_filter)
                {
                    if (key == '\r' || key == '\n')
                    {
                        view.editing_filter = false;
                    }
                    else if (key == 27)
                    {
                        view.filter = previous_filter;
                        view.editing_filter = false;
                    }
                    else if (key == 127 || key == '\b')
                    {
                        if (!view.filter.empty())
                        {
                            view.filter.pop_back();
                        }
                    }
                    else if (key >= 32 && key < 127 && view.filter.size() < 64)
                    {
                        view.filter += key;
                    }
                    view.scroll = 0;
                    continue;
                }
                if (key == 'q')
                {
                    quit = true;
                }
                else if (key >= '1' && key <= '6')
                {
                    view.page = static_cast<unsigned>(key - '0');
                    view.scroll = 0;
                }
                else if (key == 'h')
                {
                    view.help = !view.help;
                }
                else if (key == ' ')
                {
                    view.paused = !view.paused;

                    if (!view.paused && current)
                    {
                        visible = current;
                    }
                }
                else if (key == '/')
                {
                    previous_filter = view.filter;
                    view.editing_filter = true;
                }
                else if (key == 'c')
                {
                    view.sort = "cpu";
                }
                else if (key == 'm')
                {
                    view.sort = "memory";
                }
                else if (key == 'p')
                {
                    view.sort = "pid";
                }
                else if (key == 't')
                {
                    view.sort = "threads";
                }
                else if (key == 'j')
                {
                    ++view.scroll;
                }
                else if (key == 'k' && view.scroll)
                {
                    --view.scroll;
                }
                else if (key == 'g')
                {
                    view.scroll = 0;
                }
            }
        }
        now = std::chrono::steady_clock::now();

        if (now >= next_render)
        {
            view.snapshot = visible ? &*visible : nullptr;

            if (stream)
            {
                view.connection = stream->Status();
                view.bytes_received = stream->Bytes();
                view.reconnects = stream->Reconnects();
            }
            else if (replay.is_open())
            {
                view.connection = "replaying recorded telemetry";
            }
            view.age_seconds = current ? std::chrono::duration<double>(now - last).count() : -1;
            view.diagnostics = probe ? probe->Value() : nullptr;
            view.diagnostics_age = probe ? probe->AgeSeconds() : -1;
            terminal.Draw(view);
            next_render = now + std::chrono::milliseconds(100);
        }
    }
    return 0;
}
} // namespace web_htop::client
