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
    bool isOpened = true;
    Mat black_frame;

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
            this->isOpened = false;
            this->initializeStaticFrame();
            cerr << "Error: Could not open " << config.get("name") << " feed.\n";
        }
    }

    void capture() {
        if(!cap.isOpened()) return;        
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
        if(!this->isOpened) {return this->black_frame;}
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

    Mat initializeStaticFrame() {
        // Define the dimensions of the black rectangle
        const int width = 640;
        const int height = 480;
        black_frame = Mat::zeros(height, width, CV_8UC3);
        cout<<"Initializing static frame for "<<config.get("name")<<endl;

        // Define the font and scale
        int font = FONT_HERSHEY_SIMPLEX;
        double font_scale = 1.5;
        int thickness = 2;

        // Get the size of the text box
        int baseline = 0;
        Size text_size = getTextSize("No Video", font, font_scale, thickness, &baseline);

        // Calculate the text position to center it in the rectangle
        Point text_origin((width - text_size.width) / 2, (height + text_size.height) / 2);

        // Add the text to the image
        putText(black_frame, config.get("name"), Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2);
        putText(black_frame, "No Video", text_origin, font, font_scale, Scalar(255, 255, 255), thickness);
        return black_frame;
    }
};