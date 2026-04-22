#include <cassert>
#include <iostream>
#include "common/models/cpu_metrics.hpp"
#include "common/json_utils.hpp"
#include "common/protocol.hpp"
#include "server/collectors/cpu_collector.hpp"

using namespace web_htop;

void testCpuMetricsJsonSerialization() {
    std::cout << "Testing CPU metrics JSON serialization..." << std::endl;
    
    // Create CPU metrics
    models::CpuMetrics metrics;
    metrics.timestamp = 1234567890;
    metrics.core_count = 4;
    metrics.total_usage_percent = 42.5;
    metrics.frequency_mhz = 2400.0;
    metrics.per_core_usage = {10.0, 20.0, 30.0, 40.0};
    
    // Serialize to JSON
    auto json = metrics.toJson();
    std::string json_str = json.toString(2);  // Pretty print
    std::cout << "Serialized JSON:\n" << json_str << std::endl;
    
    // Deserialize back
    auto parsed = json::parse(json_str);
    assert(parsed);
    
    models::CpuMetrics restored = models::CpuMetrics::fromJson(*parsed);
    
    // Verify
    assert(restored.timestamp == metrics.timestamp);
    assert(restored.core_count == metrics.core_count);
    assert(restored.total_usage_percent == metrics.total_usage_percent);
    assert(restored.frequency_mhz == metrics.frequency_mhz);
    assert(restored.per_core_usage.size() == metrics.per_core_usage.size());
    
    for (size_t i = 0; i < restored.per_core_usage.size(); ++i) {
        assert(restored.per_core_usage[i] == metrics.per_core_usage[i]);
    }
    
    std::cout << "✓ CPU metrics JSON serialization test passed" << std::endl;
}

void testCpuMetricsMessage() {
    std::cout << "Testing CPU metrics protocol message..." << std::endl;
    
    models::CpuMetrics metrics;
    metrics.timestamp = 1000;
    metrics.core_count = 2;
    metrics.total_usage_percent = 35.0;
    metrics.frequency_mhz = 2000.0;
    
    // Create and serialize message
    protocol::CpuMetricsMessage msg(metrics);
    std::string serialized = msg.serialize();
    std::cout << "Serialized message: " << serialized << std::endl;
    
    // Deserialize
    protocol::CpuMetricsMessage restored = protocol::CpuMetricsMessage::deserialize(serialized);
    
    assert(restored.metrics.core_count == metrics.core_count);
    assert(restored.metrics.total_usage_percent == metrics.total_usage_percent);
    
    std::cout << "✓ CPU metrics protocol message test passed" << std::endl;
}

void testCpuCollector() {
    std::cout << "Testing CPU collector..." << std::endl;
    
    server::collectors::CpuCollector collector;
    
    // Core count should be > 0
    CpuCount cores = collector.getCoreCount();
    assert(cores > 0);
    std::cout << "Detected " << cores << " CPU cores" << std::endl;
    
    // First collection (baseline)
    auto metrics1 = collector.collect();
    assert(metrics1.core_count == cores);
    std::cout << "First collection - Frequency: " << metrics1.frequency_mhz << " MHz" << std::endl;
    
    // Second collection (should have usage percentage)
    auto metrics2 = collector.collect();
    assert(metrics2.core_count == cores);
    std::cout << "Second collection - CPU Usage: " << metrics2.total_usage_percent << "%" << std::endl;
    
    // Usage should be between 0 and 100
    assert(metrics2.total_usage_percent >= 0.0);
    assert(metrics2.total_usage_percent <= 100.0);
    
    std::cout << "✓ CPU collector test passed" << std::endl;
}

void testJsonParsing() {
    std::cout << "Testing JSON parsing..." << std::endl;
    
    // Test parsing a CPU metrics JSON
    std::string json_str = R"({
        "timestamp": 1234567890,
        "core_count": 8,
        "total_usage_percent": 45.5,
        "frequency_mhz": 3500.0,
        "per_core_usage": [10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0]
    })";
    
    auto parsed = json::parse(json_str);
    assert(parsed);
    
    assert(parsed->hasField("timestamp"));
    assert(parsed->hasField("core_count"));
    assert(parsed->hasField("total_usage_percent"));
    assert(parsed->hasField("frequency_mhz"));
    assert(parsed->hasField("per_core_usage"));
    
    auto ts = parsed->at("timestamp").asULongLong();
    assert(ts && *ts == 1234567890);
    
    auto cores = parsed->at("core_count").asInt();
    assert(cores && *cores == 8);
    
    auto usage = parsed->at("total_usage_percent").asDouble();
    assert(usage && *usage == 45.5);
    
    const auto* arr = parsed->at("per_core_usage").asArray();
    assert(arr && arr->size() == 8);
    
    auto first_core = arr->at(0).asDouble();
    assert(first_core && *first_core == 10.0);
    
    std::cout << "✓ JSON parsing test passed" << std::endl;
}

int main() {
    std::cout << "=== CPU Collector and JSON Serialization Tests ===" << std::endl;
    
    try {
        testJsonParsing();
        testCpuMetricsJsonSerialization();
        testCpuMetricsMessage();
        testCpuCollector();
        
        std::cout << "\n=== All tests passed! ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
