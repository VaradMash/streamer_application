#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <string>
#include <iostream>

using json = nlohmann::json;
using namespace std;

class Config {
private:
    unordered_map<string, string> config;

public:
    // Add a configuration key-value pair
    void add_config(const string& key, const string& value) {
        this->config[key] = value;
    }

    // Get a configuration value by key
    string get(const string& key) const {
        if (this->config.find(key) != this->config.end()) {
            return this->config.at(key);
        }
        return ""; // Return empty string if key doesn't exist
    }

    // Print all key-value pairs in the config
    void print() const {
        for (const auto& [key, value] : config) {
            cout << key << ": " << value << endl;
        }
    }
};

// Function to read configuration from JSON file and populate the config map
unordered_map<string, Config> read_config(const string& path) {
    unordered_map<string, Config> config_map;

    // Load the JSON file
    ifstream config_file(path);
    if (!config_file.is_open()) {
        cerr << "Error: Could not open configuration file." << endl;
        return config_map;
    }

    // Parse configuration
    json config;
    try {
        config_file >> config; // Read and parse JSON
    } catch (const json::parse_error& e) {
        cerr << "Error parsing JSON: " << e.what() << endl;
        return config_map;
    }

    // Iterate through the JSON array
    for (const auto& camera : config) {
        Config camera_config;

        // Dynamically add all key-value pairs to the Config object
        for (auto it = camera.begin(); it != camera.end(); ++it) {
            string key = it.key();
            string value = it.value().dump(); // Convert JSON value to string
            // Remove quotes around string values
            if (value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }
            camera_config.add_config(key, value);
        }

        // Use the camera ID as the key for the unordered_map
        if (camera.contains("id")) {
            string id = camera["id"];
            config_map[id] = camera_config;
        }
    }

    return config_map;
}