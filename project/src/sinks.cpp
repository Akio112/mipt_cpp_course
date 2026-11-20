#include <print>
#include <stdexcept>
#include <string>
#include <utility>

#include "alert_sink.h"
#include "dedup_sink.h"
#include "detection.h"

namespace nano_edr {

// --- ConsoleSink -----------------------------------------------------------

void ConsoleSink::operator()(const Detection& detection) const {
    std::println("{}", ToString(detection));
}

// --- FileSink --------------------------------------------------------------

FileSink::FileSink(const std::string& path) : out_(path) {
    if (!out_.is_open()) {
        throw std::runtime_error("не удалось открыть файл: " + path);
    }
}

void FileSink::operator()(const Detection& detection) {
    out_ << ToString(detection) << '\n';
}

// --- CountingSink ----------------------------------------------------------

void CountingSink::operator()(const Detection& detection) const {
    ++counter_->total;
    counter_->seen.push_back(detection);
}

// --- FanOutSink ------------------------------------------------------------

void FanOutSink::Add(AlertSink sink) {
    sinks_.push_back(std::move(sink));
}

void FanOutSink::operator()(const Detection& detection) const {
    for (const auto& sink : sinks_) {
        sink(detection);
    }
}

// --- DedupSink -------------------------------------------------------------

DedupSink::DedupSink(AlertSink inner, uint64_t period_ms,
                     std::size_t* suppressed)
    : inner_(std::move(inner)),
      period_ms_(period_ms),
      suppressed_(suppressed) {}

void DedupSink::operator()(const Detection& detection) {
    if (period_ms_ == 0) {
        inner_(detection);
        return;
    }
    const uint64_t now = detection.ts.ms;
    for (auto& seen : seen_) {
        if (seen.rule != detection.rule || seen.pid != detection.pid) continue;
        if (now >= seen.first_ms && now - seen.first_ms <= period_ms_) {
            if (suppressed_ != nullptr) {
                ++*suppressed_;
            }
            return;
        }
        seen.first_ms = now;
        inner_(detection);
        return;
    }
    seen_.push_back({detection.rule, detection.pid, now});
    inner_(detection);
}

}  // namespace nano_edr
