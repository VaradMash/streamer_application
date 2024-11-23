#include <opencv2/opencv.hpp>
#include <iostream>
#include "../config_parser/read_source_config.cpp"

using namespace cv;
using namespace std;

class FeedCapture {
private:
    Config config;
    VideoCapture cap;
    Mat frame;
    mutex mtx;  // Mutex to protect frame access
    bool isRunning = true;

public:
    explicit FeedCapture(Config config) : config(config) {}

    void initialize() {
        string local_camera = config.get("local_camera");
        if (local_camera == "true") {
            cap.open(0); // Open default local camera
        } else {
            string stream_url = config.get("url");
            cap.open(stream_url); // Open stream URL
        }

        if (!cap.isOpened()) {
            cerr << "Error: Could not open " << config.get("name") << " feed.\n";
        }
    }

    void capture() {
        while (isRunning) {
            Mat tempFrame;
            if (cap.isOpened()) {
                cap >> tempFrame; // Capture frame
                if (!tempFrame.empty()) {
                    lock_guard<mutex> lock(mtx); // Lock while updating the frame
                    frame = tempFrame.clone();
                }
            }
        }
    }

    Mat getFrame() {
        lock_guard<mutex> lock(mtx); // Lock while accessing the frame
        if (!frame.empty()) {
            Mat annotatedFrame = frame.clone();
            // Annotate the frame with the camera name
            string name = config.get("name");
            putText(annotatedFrame, name, Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2);
            return annotatedFrame;
        }
        return frame.clone();
    }

    void stop() {
        isRunning = false;
    }

    ~FeedCapture() {
        if (cap.isOpened()) {
            cap.release();
        }
    }
};