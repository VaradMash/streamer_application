#include<iostream>
#include <bits/stdc++.h>
#include <opencv2/opencv.hpp>
#include "./video_capture/video_capture_thread.cpp"

using namespace std;
using namespace cv;

void displayFeeds(vector<FeedCapture*>& feeds, const string& windowName) {
    const int rows = 2;   // Number of rows in the grid
    const int cols = 2;   // Number of columns in the grid
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
    unordered_map<string, Config> camera_configs = read_config(config_path);

     vector<FeedCapture*> feeds;
    vector<thread> threads;

    // Initialize FeedCapture objects for each camera
    for (const auto& [id, config] : camera_configs) {
        FeedCapture* feed = new FeedCapture(config);
        feed->initialize();
        feeds.push_back(feed);
        threads.emplace_back(&FeedCapture::capture, feed); // Start capture in a separate thread
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

    // Clean up
    for (auto& feed : feeds) {
        delete feed;
    }

    return 0;
}

