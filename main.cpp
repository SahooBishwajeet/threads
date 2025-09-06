// Resources - https://michael-crum.com/string_art_generator/

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

using namespace std;

const int PINS = 300;         // Number of pins around the circle
const int MIN_DISTANCE = 30;  // Minimum distance between pins to avoid short lines
const int MAX_LINES = 3500;   // Maximum number of lines to draw
const int LINE_WEIGHT = 30;   // Weight to reduce error along drawn lines
const int SCALE_FACTOR = 4;   // Scale factor for output image
const int MAX_FRAMES = 50;    // Maximum frames for animation

struct Coord {
    double x;
    double y;
};

class StringArtGenerator {
   private:
    int imgSize;
    vector<Coord> pinCoords;
    vector<vector<int>> lineCacheY;
    vector<vector<int>> lineCacheX;
    cv::Mat sourceImage;
    cv::Mat error;

    void calculatePinCoords() {
        double center = imgSize / 2.0;
        double radius = (imgSize / 2.0) - 1;

        pinCoords.resize(PINS);
        for (int i = 0; i < PINS; i++) {
            double angle = 2 * M_PI * i / PINS;
            pinCoords[i].x = floor(center + radius * cos(angle));
            pinCoords[i].y = floor(center + radius * sin(angle));
        }
    }

    void precalculateAllPotentialLines() {
        lineCacheY.resize(PINS * PINS);
        lineCacheX.resize(PINS * PINS);

        for (int i = 0; i < PINS; i++) {
            for (int j = i + MIN_DISTANCE; j < PINS; j++) {
                double x0 = pinCoords[i].x;
                double y0 = pinCoords[i].y;
                double x1 = pinCoords[j].x;
                double y1 = pinCoords[j].y;

                double d = floor(sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0)));
                vector<int> xs, ys;

                // Linear interpolation
                for (int k = 0; k < d; k++) {
                    double t = k / d;
                    xs.push_back(floor(x0 + t * (x1 - x0)));
                    ys.push_back(floor(y0 + t * (y1 - y0)));
                }

                lineCacheY[j * PINS + i] = ys;
                lineCacheY[i * PINS + j] = ys;
                lineCacheX[j * PINS + i] = xs;
                lineCacheX[i * PINS + j] = xs;
            }
        }
    }

    double getLineErr(const cv::Mat& err, const vector<int>& coords1, const vector<int>& coords2) {
        double sum = 0;
        for (size_t i = 0; i < coords1.size(); i++) {
            sum += err.at<uchar>(coords1[i], coords2[i]);
        }
        return sum;
    }

   public:
    StringArtGenerator(const string& imagePath) {
        sourceImage = cv::imread(imagePath, cv::IMREAD_GRAYSCALE);
        if (sourceImage.empty()) {
            throw runtime_error("Error: Could not read the image.");
        }

        imgSize = max(sourceImage.cols, sourceImage.rows);
        cv::resize(sourceImage, sourceImage, cv::Size(imgSize, imgSize));

        cv::Mat mask = cv::Mat::zeros(imgSize, imgSize, CV_8UC1);
        cv::circle(mask, cv::Point(imgSize / 2, imgSize / 2), imgSize / 2, cv::Scalar(255), -1);
        cv::bitwise_and(sourceImage, mask, sourceImage);

        calculatePinCoords();
        precalculateAllPotentialLines();
    }

    vector<int> generateStringArt() {
        vector<int> lineSequence;
        error = cv::Mat::ones(sourceImage.size(), CV_8UC1) * 255 - sourceImage;

        int currentPin = 0;
        vector<int> lastPins(20, 0);
        lineSequence.push_back(currentPin);

        cout << "Generating string art..." << endl;
        for (int i = 0; i < MAX_LINES; i++) {
            int bestPin = -1;
            double maxErr = 0;

            for (int offset = MIN_DISTANCE; offset < PINS - MIN_DISTANCE; offset++) {
                int testPin = (currentPin + offset) % PINS;

                // Skip if pin was recently used
                if (find(lastPins.begin(), lastPins.end(), testPin) != lastPins.end()) {
                    continue;
                }

                int index = testPin * PINS + currentPin;
                double lineErr = getLineErr(error, lineCacheY[index], lineCacheX[index]);

                if (lineErr > maxErr) {
                    maxErr = lineErr;
                    bestPin = testPin;
                }
            }

            if (bestPin == -1) break;

            lineSequence.push_back(bestPin);
            int index = bestPin * PINS + currentPin;

            for (size_t i = 0; i < lineCacheY[index].size(); i++) {
                error.at<uchar>(lineCacheY[index][i], lineCacheX[index][i]) = max(
                    0, error.at<uchar>(lineCacheY[index][i], lineCacheX[index][i]) - LINE_WEIGHT);
            }

            lastPins.erase(lastPins.begin());
            lastPins.push_back(bestPin);
            currentPin = bestPin;

            if ((i + 1) % 100 == 0) {
                cout << "Progress: " << i + 1 << "/" << MAX_LINES << " lines" << endl;
            }
        }

        return lineSequence;
    }

    void saveResult(const vector<int>& sequence, const string& outputPath) {
        cv::Mat result =
            cv::Mat::ones(imgSize * SCALE_FACTOR, imgSize * SCALE_FACTOR, CV_8UC1) * 255;

        for (size_t i = 0; i < sequence.size() - 1; i++) {
            int startPin = sequence[i];
            int endPin = sequence[i + 1];

            cv::Point start(static_cast<int>(pinCoords[startPin].x * SCALE_FACTOR),
                            static_cast<int>(pinCoords[startPin].y * SCALE_FACTOR));
            cv::Point end(static_cast<int>(pinCoords[endPin].x * SCALE_FACTOR),
                          static_cast<int>(pinCoords[endPin].y * SCALE_FACTOR));

            cv::line(result, start, end, cv::Scalar(0), 1, cv::LINE_AA);
        }

        cv::imwrite(outputPath, result);
    }

    void saveSequenceToFile(const vector<int>& sequence, const string& filePath) {
        ofstream outFile(filePath);
        if (!outFile) {
            throw runtime_error("Error: Could not create sequence file.");
        }

        for (size_t i = 0; i < sequence.size(); ++i) {
            outFile << sequence[i];
            if (i < sequence.size() - 1) {
                outFile << ",";
            }
        }
    }

    void saveAnimation(const vector<int>& sequence, const string& outputPath) {
        cv::Mat result =
            cv::Mat::ones(imgSize * SCALE_FACTOR, imgSize * SCALE_FACTOR, CV_8UC1) * 255;

        int frameCount = 0;
        int frameStep = max(1, static_cast<int>(sequence.size() / MAX_FRAMES));

        string baseName = outputPath.substr(0, outputPath.find_last_of("."));
        string videoPath = baseName + "_animation.avi";

        int codec = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');  // .avi with MJPEG codec
        double fps = 30.0;
        cv::Size frameSize(result.cols, result.rows);

        cv::VideoWriter writer(videoPath, codec, fps, frameSize, false);  // false = grayscale

        if (!writer.isOpened()) {
            cerr << "Error: Could not open the video file for writing." << endl;
            return;
        }

        for (size_t i = 1; i < sequence.size(); i++) {
            int startPin = sequence[i - 1];
            int endPin = sequence[i];

            cv::Point start(static_cast<int>(pinCoords[startPin].x * SCALE_FACTOR),
                            static_cast<int>(pinCoords[startPin].y * SCALE_FACTOR));
            cv::Point end(static_cast<int>(pinCoords[endPin].x * SCALE_FACTOR),
                          static_cast<int>(pinCoords[endPin].y * SCALE_FACTOR));

            cv::line(result, start, end, cv::Scalar(0), 1, cv::LINE_AA);

            if (i % frameStep == 0 || i == sequence.size() - 1) {
                writer.write(result);
                frameCount++;
            }
        }

        writer.release();
        cout << "Animation video saved to: " << videoPath << endl;
    }
};

int main(int argc, char** argv) {
    if (argc < 3 || argc > 5) {
        cout << "Usage: " << argv[0] << " <input_image> <output_image> [sequence_file] [-a]" << endl;
        cout << "Options:" << endl;
        cout << "  sequence_file  : Optional file to save the pin sequence" << endl;
        cout << "  -a             : Generate animation frames" << endl;
        return -1;
    }

    bool generateAnimation = false;
    string sequenceFile;

    for (int i = 3; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-a") {
            generateAnimation = true;
        } else {
            sequenceFile = arg;
        }
    }

    try {
        StringArtGenerator generator(argv[1]);
        vector<int> sequence = generator.generateStringArt();
        generator.saveResult(sequence, argv[2]);

        if (!sequenceFile.empty()) {
            generator.saveSequenceToFile(sequence, sequenceFile);
            cout << "Sequence saved to: " << sequenceFile << endl;
        }

        if (generateAnimation) {
            generator.saveAnimation(sequence, argv[2]);
        }
        cout << "String art generated successfully!" << endl;
    } catch (const exception& e) {
        cerr << e.what() << endl;
        return -1;
    }

    return 0;
}
