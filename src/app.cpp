#include <iostream>
#include <thread>
#include <unordered_map>
#include <opencv2/opencv.hpp>
#include "./video_capture/video_capture_thread.cpp"

using namespace std;
using namespace cv;

// Function to get model weights
Net get_model_weights(const string& model_weights, const string& model_cfg, bool use_hardware_acceleration) {
    Net net = readNet(model_weights, model_cfg);
    if (use_hardware_acceleration) {
        net.setPreferableBackend(DNN_BACKEND_OPENCV);
        net.setPreferableTarget(DNN_TARGET_CPU);
    }
    return net;
}

// Function to display feeds in a grid
void displayFeeds(vector<unique_ptr<FeedCapture>>& feeds, const string& windowName) {
    const int rows = 2;          // Number of rows in the grid
    const int cols = 2;          // Number of columns in the grid
    const int cellWidth = 640;   // Width of each cell in the grid
    const int cellHeight = 480;  // Height of each cell in the grid

    Mat display(rows * cellHeight, cols * cellWidth, CV_8UC3, Scalar(0, 0, 0)); // Composite frame

    while (true) {
        int i = 0;
        for (auto& feed : feeds) {
            Mat frame = feed->getFrame();
            if (!frame.empty()) {
                resize(frame, frame, Size(cellWidth, cellHeight)); // Resize to fit grid cell

                int x = (i % cols) * cellWidth;  // X position in composite frame
                int y = (i / cols) * cellHeight; // Y position in composite frame

                frame.copyTo(display(Rect(x, y, cellWidth, cellHeight))); // Place frame in grid
            }
            i++;
        }

        imshow(windowName, display);

        // Exit on 'q' key press
        if (waitKey(30) == 'q') {
            break;
        }
    }

    destroyAllWindows();
}

int main() {
    string config_path = "./config/camera-source-config.json"; // Path to the configuration file
    bool enable_object_detection = true;

    // Assuming read_config is a function that reads and parses the configuration
    unordered_map<string, Config> camera_configs = read_config(config_path);

    vector<unique_ptr<FeedCapture>> feeds;
    vector<thread> threads;

    // Load YOLO model
    Net net = get_model_weights("./model/yolov4-tiny.weights", "./model/yolov4-tiny.cfg", false);

    // Load class names
    vector<string> class_names;
    ifstream ifs("./model/coco.names");
    string line;
    while (getline(ifs, line)) {
        class_names.push_back(line);
    }

    // Initialize FeedCapture objects for each camera
    for (const auto& [id, config] : camera_configs) {
        auto feed = make_unique<FeedCapture>(config, net, enable_object_detection, class_names);
        feed->initialize();
        threads.emplace_back(&FeedCapture::capture, feed.get()); // Start capture in a separate thread
        feeds.push_back(move(feed)); // Move unique_ptr to the vector
    }

    // Display all feeds in a single window
    displayFeeds(feeds, "Camera Feeds");

    // Stop all capture threads
    for (auto& feed : feeds) {
        feed->stop();
    }

    // Wait for threads to finish
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    return 0;
}
