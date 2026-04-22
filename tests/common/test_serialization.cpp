/**
 * @file tests/common/test_serialization.cpp
 *
 * @author Maksim Vashkevich
 * @date 2026-04-18
 * @note Updated 2026-04-20.
 *
 * @brief Unit tests for metrics JSON serialization/deserialization.
 * @details Verifies correctness of `ToJson` and `FromJson` for all metric
 * models from `common/models`, including nested arrays and process list data.
 */

 #include <gtest/gtest.h>

 #include "common/models/cpu_metrics.hpp"
 #include "common/models/disk_metrics.hpp"
 #include "common/models/loadavg_metrics.hpp"
 #include "common/models/memory_metrics.hpp"
 #include "common/models/network_metrics.hpp"
 #include "common/models/process_metrics.hpp"
 #include "common/models/system_snapshot.hpp"
 
 #include <string_view>
 
 namespace {
 
 /**
  * @test SerializationTest.VerifyCPUMetricsToJsonAndFromJson
  * @brief Verifies CPU metrics serialization and round-trip restoration.
  * @details Checks all CPU JSON fields including per-core usage array and
  * validates equality after `ToJson` -> `FromJson`.
  */
 TEST(SerializationTest, VerifyCPUMetricsToJsonAndFromJson) {
     web_htop::models::CPUMetrics metrics{};
     metrics.timestamp              = 1713427200000ULL;
     metrics.core_count             = 8;
     metrics.total_usage_percent    = 63.5;
     metrics.per_core_usage_percent = {58.0, 61.5, 63.0, 64.0, 66.5, 62.0, 65.0, 68.0};
     metrics.frequency_mhz          = 2450.25;
 
     auto const json = metrics.ToJson();
     ASSERT_TRUE(json.IsObject());
     ASSERT_EQ(json.size(), 5U);
 
     auto timestamp = json["timestamp"];
     ASSERT_TRUE(timestamp.has_value());
     ASSERT_TRUE(timestamp->get().AsUInt64().has_value());
     EXPECT_EQ(timestamp->get().AsUInt64().value(), metrics.timestamp);
 
     auto core_count = json["core_count"];
     ASSERT_TRUE(core_count.has_value());
     ASSERT_TRUE(core_count->get().AsUInt64().has_value());
     EXPECT_EQ(core_count->get().AsUInt64().value(), metrics.core_count);
 
     auto total_usage = json["total_usage_percent"];
     ASSERT_TRUE(total_usage.has_value());
     ASSERT_TRUE(total_usage->get().AsDouble().has_value());
     EXPECT_DOUBLE_EQ(total_usage->get().AsDouble().value(), metrics.total_usage_percent);
 
     auto frequency = json["frequency_mhz"];
     ASSERT_TRUE(frequency.has_value());
     ASSERT_TRUE(frequency->get().AsDouble().has_value());
     EXPECT_DOUBLE_EQ(frequency->get().AsDouble().value(), metrics.frequency_mhz);
 
     auto per_core_usage = json["per_core_usage_percent"];
     ASSERT_TRUE(per_core_usage.has_value());
     auto const * per_core_array = per_core_usage->get().AsArray();
     ASSERT_NE(per_core_array, nullptr);
     ASSERT_EQ(per_core_array->size(), metrics.per_core_usage_percent.size());
     for (size_t i = 0; i < per_core_array->size(); i++) {
         ASSERT_TRUE(per_core_array->at(i).AsDouble().has_value());
         EXPECT_DOUBLE_EQ(per_core_array->at(i).AsDouble().value(), metrics.per_core_usage_percent[i]);
     }
 
     auto const restored = web_htop::models::CPUMetrics::FromJson(json);
     EXPECT_EQ(restored.timestamp, metrics.timestamp);
     EXPECT_EQ(restored.core_count, metrics.core_count);
     EXPECT_DOUBLE_EQ(restored.total_usage_percent, metrics.total_usage_percent);
     EXPECT_DOUBLE_EQ(restored.frequency_mhz, metrics.frequency_mhz);
     ASSERT_EQ(restored.per_core_usage_percent.size(), metrics.per_core_usage_percent.size());
     for (size_t i = 0; i < restored.per_core_usage_percent.size(); i++) {
         EXPECT_DOUBLE_EQ(restored.per_core_usage_percent[i], metrics.per_core_usage_percent[i]);
     }
 }
 
 /**
  * @test SerializationTest.VerifyMemoryMetricsToJsonAndFromJson
  * @brief Verifies memory metrics serialization and round-trip restoration.
  * @details Ensures memory capacity and usage fields are serialized with proper
  * numeric types and recovered without data loss.
  */
 TEST(SerializationTest, VerifyMemoryMetricsToJsonAndFromJson) {
     web_htop::models::MemoryMetrics metrics{};
     metrics.timestamp       = 1713427200123ULL;
     metrics.total_bytes     = 34ULL * 1024ULL * 1024ULL * 1024ULL;
     metrics.available_bytes = 12ULL * 1024ULL * 1024ULL * 1024ULL;
     metrics.used_bytes      = 22ULL * 1024ULL * 1024ULL * 1024ULL;
     metrics.used_percent    = 64.7058823529;
 
     auto const json = metrics.ToJson();
     ASSERT_TRUE(json.IsObject());
     ASSERT_EQ(json.size(), 5U);
 
     auto used_percent = json["used_percent"];
     ASSERT_TRUE(used_percent.has_value());
     ASSERT_TRUE(used_percent->get().AsDouble().has_value());
     EXPECT_DOUBLE_EQ(used_percent->get().AsDouble().value(), metrics.used_percent);
 
     auto const restored = web_htop::models::MemoryMetrics::FromJson(json);
     EXPECT_EQ(restored.timestamp, metrics.timestamp);
     EXPECT_EQ(restored.total_bytes, metrics.total_bytes);
     EXPECT_EQ(restored.available_bytes, metrics.available_bytes);
     EXPECT_EQ(restored.used_bytes, metrics.used_bytes);
     EXPECT_DOUBLE_EQ(restored.used_percent, metrics.used_percent);
 }
 
 /**
  * @test SerializationTest.VerifyDiskMetricsToJsonAndFromJson
  * @brief Verifies disk metrics serialization and round-trip restoration.
  * @details Confirms disk capacity/usage fields are present in JSON and parsed
  * back into equivalent `DiskMetrics`.
  */
 TEST(SerializationTest, VerifyDiskMetricsToJsonAndFromJson) {
     web_htop::models::DiskMetrics metrics{};
     metrics.timestamp       = 1713427200456ULL;
     metrics.total_bytes     = 512ULL * 1024ULL * 1024ULL * 1024ULL;
     metrics.available_bytes = 320ULL * 1024ULL * 1024ULL * 1024ULL;
     metrics.used_bytes      = 192ULL * 1024ULL * 1024ULL * 1024ULL;
     metrics.used_percent    = 37.5;
 
     auto const json = metrics.ToJson();
     ASSERT_TRUE(json.IsObject());
     ASSERT_EQ(json.size(), 5U);
 
     auto total_bytes = json["total_bytes"];
     ASSERT_TRUE(total_bytes.has_value());
     ASSERT_TRUE(total_bytes->get().AsUInt64().has_value());
     EXPECT_EQ(total_bytes->get().AsUInt64().value(), metrics.total_bytes);
 
     auto const restored = web_htop::models::DiskMetrics::FromJson(json);
     EXPECT_EQ(restored.timestamp, metrics.timestamp);
     EXPECT_EQ(restored.total_bytes, metrics.total_bytes);
     EXPECT_EQ(restored.available_bytes, metrics.available_bytes);
     EXPECT_EQ(restored.used_bytes, metrics.used_bytes);
     EXPECT_DOUBLE_EQ(restored.used_percent, metrics.used_percent);
 }
 
 /**
  * @test SerializationTest.VerifyNetworkMetricsToJsonAndFromJson
  * @brief Verifies network metrics serialization and round-trip restoration.
  * @details Validates total traffic counters and throughput fields in JSON and
  * checks restored `NetworkMetrics` equality.
  */
 TEST(SerializationTest, VerifyNetworkMetricsToJsonAndFromJson) {
     web_htop::models::NetworkMetrics metrics{};
     metrics.timestamp      = 1713427200789ULL;
     metrics.rx_bytes_total = 987654321ULL;
     metrics.tx_bytes_total = 123456789ULL;
     metrics.rx_kbps        = 512.75;
     metrics.tx_kbps        = 208.125;
 
     auto const json = metrics.ToJson();
     ASSERT_TRUE(json.IsObject());
     ASSERT_EQ(json.size(), 5U);
 
     auto rx_kbps = json["rx_kbps"];
     ASSERT_TRUE(rx_kbps.has_value());
     ASSERT_TRUE(rx_kbps->get().AsDouble().has_value());
     EXPECT_DOUBLE_EQ(rx_kbps->get().AsDouble().value(), metrics.rx_kbps);
 
     auto const restored = web_htop::models::NetworkMetrics::FromJson(json);
     EXPECT_EQ(restored.timestamp, metrics.timestamp);
     EXPECT_EQ(restored.rx_bytes_total, metrics.rx_bytes_total);
     EXPECT_EQ(restored.tx_bytes_total, metrics.tx_bytes_total);
     EXPECT_DOUBLE_EQ(restored.rx_kbps, metrics.rx_kbps);
     EXPECT_DOUBLE_EQ(restored.tx_kbps, metrics.tx_kbps);
 }
 
 /**
  * @test SerializationTest.VerifyProcessInfoToJsonAndFromJson
  * @brief Verifies per-process entry serialization and restoration.
  * @details Checks JSON encoding of PID, process name, state, CPU/memory usage,
  * and thread count for `ProcessInfo`.
  */
 TEST(SerializationTest, VerifyProcessInfoToJsonAndFromJson) {
     web_htop::models::ProcessInfo process{};
     process.pid            = 4242;
     process.name           = "web_htop_worker";
     process.state          = web_htop::ProcessState::RUNNING;
     process.cpu_percent    = 11.75;
     process.memory_bytes   = 78ULL * 1024ULL * 1024ULL;
     process.memory_percent = 2.5;
     process.thread_count   = 7;
 
     auto const json = process.ToJson();
     ASSERT_TRUE(json.IsObject());
     ASSERT_EQ(json.size(), 7U);
 
     auto name = json["name"];
     ASSERT_TRUE(name.has_value());
     ASSERT_TRUE(name->get().AsString().has_value());
     EXPECT_EQ(name->get().AsString().value(), std::string_view("web_htop_worker"));
 
     auto state = json["state"];
     ASSERT_TRUE(state.has_value());
     ASSERT_TRUE(state->get().AsInt64().has_value());
     EXPECT_EQ(state->get().AsInt64().value(), static_cast<std::int64_t>('R'));
 
     auto const restored = web_htop::models::ProcessInfo::FromJson(json);
     EXPECT_EQ(restored.pid, process.pid);
     EXPECT_EQ(restored.name, process.name);
     EXPECT_EQ(restored.state, process.state);
     EXPECT_DOUBLE_EQ(restored.cpu_percent, process.cpu_percent);
     EXPECT_EQ(restored.memory_bytes, process.memory_bytes);
     EXPECT_DOUBLE_EQ(restored.memory_percent, process.memory_percent);
     EXPECT_EQ(restored.thread_count, process.thread_count);
 }
 
 /**
  * @test SerializationTest.VerifyProcessMetricsToJsonAndFromJson
  * @brief Verifies process list metrics serialization and restoration.
  * @details Ensures top-level process counters and nested `processes` array are
  * serialized correctly and reconstructed into equivalent `ProcessMetrics`.
  */
 TEST(SerializationTest, VerifyProcessMetricsToJsonAndFromJson) {
     web_htop::models::ProcessInfo process_1{};
     process_1.pid            = 1001;
     process_1.name           = "init";
     process_1.state          = web_htop::ProcessState::SLEEPING;
     process_1.cpu_percent    = 0.1;
     process_1.memory_bytes   = 15ULL * 1024ULL * 1024ULL;
     process_1.memory_percent = 0.4;
     process_1.thread_count   = 1;
 
     web_htop::models::ProcessInfo process_2{};
     process_2.pid            = 2022;
     process_2.name           = "renderer";
     process_2.state          = web_htop::ProcessState::RUNNING;
     process_2.cpu_percent    = 18.5;
     process_2.memory_bytes   = 250ULL * 1024ULL * 1024ULL;
     process_2.memory_percent = 3.8;
     process_2.thread_count   = 12;
 
     web_htop::models::ProcessMetrics metrics{};
     metrics.timestamp         = 1713427200999ULL;
     metrics.total_processes   = 2;
     metrics.running_processes = 1;
     metrics.processes         = {process_1, process_2};
 
     auto const json = metrics.ToJson();
     ASSERT_TRUE(json.IsObject());
     ASSERT_EQ(json.size(), 4U);
 
     auto processes = json["processes"];
     ASSERT_TRUE(processes.has_value());
     auto const * processes_array = processes->get().AsArray();
     ASSERT_NE(processes_array, nullptr);
     ASSERT_EQ(processes_array->size(), 2U);
 
     auto const restored = web_htop::models::ProcessMetrics::FromJson(json);
     EXPECT_EQ(restored.timestamp, metrics.timestamp);
     EXPECT_EQ(restored.total_processes, metrics.total_processes);
     EXPECT_EQ(restored.running_processes, metrics.running_processes);
     ASSERT_EQ(restored.processes.size(), metrics.processes.size());
 
     for (size_t i = 0; i < restored.processes.size(); i++) {
         EXPECT_EQ(restored.processes[i].pid, metrics.processes[i].pid);
         EXPECT_EQ(restored.processes[i].name, metrics.processes[i].name);
         EXPECT_EQ(restored.processes[i].state, metrics.processes[i].state);
         EXPECT_DOUBLE_EQ(restored.processes[i].cpu_percent, metrics.processes[i].cpu_percent);
         EXPECT_EQ(restored.processes[i].memory_bytes, metrics.processes[i].memory_bytes);
         EXPECT_DOUBLE_EQ(restored.processes[i].memory_percent, metrics.processes[i].memory_percent);
         EXPECT_EQ(restored.processes[i].thread_count, metrics.processes[i].thread_count);
     }
 }
 
 /**
  * @test SerializationTest.VerifyLoadavgMetricsToJsonAndFromJson
  * @brief Verifies load average metrics serialization and restoration.
  * @details Checks 1/5/15 minute load fields in JSON and validates
  * round-trip conversion for `LoadavgMetrics`.
  */
 TEST(SerializationTest, VerifyLoadavgMetricsToJsonAndFromJson) {
     web_htop::models::LoadavgMetrics metrics{};
     metrics.timestamp = 1713427201111ULL;
     metrics.load_1m   = 0.52;
     metrics.load_5m   = 0.78;
     metrics.load_15m  = 1.04;
 
     auto const json = metrics.ToJson();
     ASSERT_TRUE(json.IsObject());
     ASSERT_EQ(json.size(), 4U);
 
     auto load_1m = json["load_1m"];
     ASSERT_TRUE(load_1m.has_value());
     ASSERT_TRUE(load_1m->get().AsDouble().has_value());
     EXPECT_DOUBLE_EQ(load_1m->get().AsDouble().value(), metrics.load_1m);
 
     auto const restored = web_htop::models::LoadavgMetrics::FromJson(json);
     EXPECT_EQ(restored.timestamp, metrics.timestamp);
     EXPECT_DOUBLE_EQ(restored.load_1m, metrics.load_1m);
     EXPECT_DOUBLE_EQ(restored.load_5m, metrics.load_5m);
     EXPECT_DOUBLE_EQ(restored.load_15m, metrics.load_15m);
 }
 
 /**
  * @test SerializationTest.VerifySystemSnapshotToJsonAndFromJson
  * @brief Verifies full system snapshot serialization and restoration.
  * @details Ensures top-level sections and nested metric values survive
  * `ToJson` -> `FromJson` round-trip for `SystemSnapshot`.
  */
 TEST(SerializationTest, VerifySystemSnapshotToJsonAndFromJson) {
     web_htop::models::SystemSnapshot snapshot{};
     snapshot.timestamp                  = 1713427201222ULL;
     snapshot.cpu.timestamp              = 1713427201222ULL;
     snapshot.cpu.core_count             = 4;
     snapshot.cpu.total_usage_percent    = 41.25;
     snapshot.cpu.per_core_usage_percent = {35.0, 40.5, 44.0, 45.5};
     snapshot.cpu.frequency_mhz          = 2345.0;
 
     snapshot.memory.timestamp       = 1713427201222ULL;
     snapshot.memory.total_bytes     = 16ULL * 1024ULL * 1024ULL * 1024ULL;
     snapshot.memory.available_bytes = 6ULL * 1024ULL * 1024ULL * 1024ULL;
     snapshot.memory.used_bytes      = 10ULL * 1024ULL * 1024ULL * 1024ULL;
     snapshot.memory.used_percent    = 62.5;
 
     snapshot.disk.timestamp       = 1713427201222ULL;
     snapshot.disk.total_bytes     = 512ULL * 1024ULL * 1024ULL * 1024ULL;
     snapshot.disk.available_bytes = 256ULL * 1024ULL * 1024ULL * 1024ULL;
     snapshot.disk.used_bytes      = 256ULL * 1024ULL * 1024ULL * 1024ULL;
     snapshot.disk.used_percent    = 50.0;
 
     snapshot.network.timestamp      = 1713427201222ULL;
     snapshot.network.rx_bytes_total = 111111ULL;
     snapshot.network.tx_bytes_total = 222222ULL;
     snapshot.network.rx_kbps        = 64.5;
     snapshot.network.tx_kbps        = 48.25;
 
     snapshot.loadavg.timestamp = 1713427201222ULL;
     snapshot.loadavg.load_1m   = 1.11;
     snapshot.loadavg.load_5m   = 1.22;
     snapshot.loadavg.load_15m  = 1.33;
 
     auto const json = snapshot.ToJson();
     ASSERT_TRUE(json.IsObject());
    ASSERT_EQ(json.size(), 7U);
 
     auto cpu = json["cpu"];
     ASSERT_TRUE(cpu.has_value());
     ASSERT_TRUE(cpu->get().IsObject());
 
     auto loadavg = json["loadavg"];
     ASSERT_TRUE(loadavg.has_value());
     ASSERT_TRUE(loadavg->get().IsObject());
 
     auto const restored = web_htop::models::SystemSnapshot::FromJson(json);
     EXPECT_EQ(restored.timestamp, snapshot.timestamp);
     EXPECT_EQ(restored.cpu.timestamp, snapshot.cpu.timestamp);
     EXPECT_EQ(restored.cpu.core_count, snapshot.cpu.core_count);
     EXPECT_DOUBLE_EQ(restored.cpu.total_usage_percent, snapshot.cpu.total_usage_percent);
     ASSERT_EQ(restored.cpu.per_core_usage_percent.size(), snapshot.cpu.per_core_usage_percent.size());
     for (size_t i = 0; i < restored.cpu.per_core_usage_percent.size(); i++) {
         EXPECT_DOUBLE_EQ(restored.cpu.per_core_usage_percent[i], snapshot.cpu.per_core_usage_percent[i]);
     }
     EXPECT_DOUBLE_EQ(restored.cpu.frequency_mhz, snapshot.cpu.frequency_mhz);
 
     EXPECT_EQ(restored.memory.total_bytes, snapshot.memory.total_bytes);
     EXPECT_EQ(restored.memory.available_bytes, snapshot.memory.available_bytes);
     EXPECT_EQ(restored.memory.used_bytes, snapshot.memory.used_bytes);
     EXPECT_DOUBLE_EQ(restored.memory.used_percent, snapshot.memory.used_percent);
 
     EXPECT_EQ(restored.disk.total_bytes, snapshot.disk.total_bytes);
     EXPECT_EQ(restored.disk.available_bytes, snapshot.disk.available_bytes);
     EXPECT_EQ(restored.disk.used_bytes, snapshot.disk.used_bytes);
     EXPECT_DOUBLE_EQ(restored.disk.used_percent, snapshot.disk.used_percent);
 
     EXPECT_EQ(restored.network.rx_bytes_total, snapshot.network.rx_bytes_total);
     EXPECT_EQ(restored.network.tx_bytes_total, snapshot.network.tx_bytes_total);
     EXPECT_DOUBLE_EQ(restored.network.rx_kbps, snapshot.network.rx_kbps);
     EXPECT_DOUBLE_EQ(restored.network.tx_kbps, snapshot.network.tx_kbps);
 
     EXPECT_DOUBLE_EQ(restored.loadavg.load_1m, snapshot.loadavg.load_1m);
     EXPECT_DOUBLE_EQ(restored.loadavg.load_5m, snapshot.loadavg.load_5m);
     EXPECT_DOUBLE_EQ(restored.loadavg.load_15m, snapshot.loadavg.load_15m);
 }
 
 } // namespace