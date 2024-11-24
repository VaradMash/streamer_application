#include <opencv2/opencv.hpp>
#include <iostream>
#include <mutex>
#include <vector>
#include "../config_parser/read_source_config.cpp"

using namespace cv;
using namespace std;
using namespace dnn;

// Constants
constexpr float CONFIDENCE_THRESHOLD = 0.5f;
constexpr float NMS_THRESHOLD = 0.4f;

class FeedCapture {
private:
    Config config;
    VideoCapture cap;
    Mat frame;
    mutex mtx;                // Mutex to protect frame access
    bool isRunning = true;    // Control flag for the capture thread
    bool isOpened = true;     // Whether the feed is successfully opened
    Mat black_frame;          // Placeholder frame for unavailable feeds
    Net& net;                 // Shared YOLO model
    bool enable_object_detection;
    vector<string> class_names;

public:
    FeedCapture(Config config, Net& net, bool enable_object_detection, const vector<string>& class_names)
        : config(std::move(config)),
          net(net),
          enable_object_detection(enable_object_detection),
          class_names(class_names) {
        initializeStaticFrame();
    }

    void initialize() {
        string local_camera = config.get("local_camera");
        if (local_camera == "true") {
            cap.open(0); // Open default local camera
        } else {
            string stream_url = config.get("url");
            cap.open(stream_url); // Open stream URL
        }

        if (!cap.isOpened()) {
            isOpened = false;
            cerr << "Error: Could not open " << config.get("name") << " feed.\n";
        }
    }

    void capture() {
        if (!cap.isOpened()) {
            return;
        }
        while (isRunning) {
            Mat temp_frame;
            cap >> temp_frame; // Capture frame
            if (temp_frame.empty()) {
                cerr << "Error: Empty frame from " << config.get("name") << ".\n";
                continue;
            }

            // Perform object detection if enabled
            if (enable_object_detection) {
                temp_frame = runObjectDetection(temp_frame);
            }

            {
                lock_guard<mutex> lock(mtx); // Lock while updating the frame
                frame = temp_frame.clone();
            }
        }
    }

    Mat runObjectDetection(Mat& input_frame) {
        Mat blob;
        blobFromImage(input_frame, blob, 1 / 255.0, Size(416, 416), Scalar(), true, false);

        net.setInput(blob);

        vector<Mat> outs;
        net.forward(outs, net.getUnconnectedOutLayersNames());

        vector<int> class_ids;
        vector<float> confidences;
        vector<Rect> boxes;

        for (const auto& output : outs) {
            for (int i = 0; i < output.rows; ++i) {
                Mat scores = output.row(i).colRange(5, output.cols);
                Point class_id_point;
                double confidence;
                minMaxLoc(scores, 0, &confidence, 0, &class_id_point);

                if (confidence > CONFIDENCE_THRESHOLD) {
                    int center_x = static_cast<int>(output.at<float>(i, 0) * input_frame.cols);
                    int center_y = static_cast<int>(output.at<float>(i, 1) * input_frame.rows);
                    int width = static_cast<int>(output.at<float>(i, 2) * input_frame.cols);
                    int height = static_cast<int>(output.at<float>(i, 3) * input_frame.rows);

                    int left = center_x - width / 2;
                    int top = center_y - height / 2;

                    boxes.emplace_back(left, top, width, height);
                    confidences.push_back(static_cast<float>(confidence));
                    class_ids.push_back(class_id_point.x);
                }
            }
        }

        vector<int> indices;
        NMSBoxes(boxes, confidences, CONFIDENCE_THRESHOLD, NMS_THRESHOLD, indices);

        for (int idx : indices) {
            Rect box = boxes[idx];
            string label = format("%.2f", confidences[idx]);
            if (!class_names.empty() && class_ids[idx] < class_names.size()) {
                label = class_names[class_ids[idx]] + ": " + label;
            }

            rectangle(input_frame, box, Scalar(0, 255, 0), 2);
            putText(input_frame, label, Point(box.x, box.y - 5), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 1);
        }

        return input_frame;
    }

    Mat getFrame() {
        lock_guard<mutex> lock(mtx); // Lock while accessing the frame
        if (!isOpened) {
            return black_frame.clone();
        }
        if (!frame.empty()) {
            Mat annotated_frame = frame.clone();
            putText(annotated_frame, config.get("name"), Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2);
            return annotated_frame;
        }
        return black_frame.clone();
    }

    void stop() {
        isRunning = false;
    }

    ~FeedCapture() {
        if (cap.isOpened()) {
            cap.release();
        }
    }

private:
    void initializeStaticFrame() {
        const int width = 640;
        const int height = 480;
        black_frame = Mat::zeros(height, width, CV_8UC3);

        putText(black_frame, "No Video", Point(width / 4, height / 2), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
        putText(black_frame, config.get("name"), Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2);
    }
};
