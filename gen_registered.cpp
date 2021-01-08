#include <ultimateALPR-SDK-API-PUBLIC.h>
#include <alpr_utils.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <json.hpp> // nlohmann/json
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm> // std::replace
using namespace ultimateAlprSdk;
// Configuration for ANPR deep learning engine
static const char* __jsonConfig =
"{"
"\"debug_level\": \"fatal\","
"\"debug_write_input_image_enabled\": false,"
"\"debug_internal_data_path\": \".\","
""
"\"num_threads\": -1,"
"\"gpgpu_enabled\": true,"
""
"\"klass_vcr_gamma\": 1.5,"
""
"\"detect_roi\": [600, 1200, 0, 600],"
"\"detect_minscore\": 0.1,"
""
"\"pyramidal_search_enabled\": false,"
"\"pyramidal_search_sensitivity\": 0.28,"
"\"pyramidal_search_minscore\": 0.3,"
"\"pyramidal_search_min_image_size_inpixels\": 800,"
""
"\"recogn_minscore\": 0.3,"
"\"recogn_score_type\": \"min\","
""
"\"assets_folder\": \"../assets\","
"\"license_token_data\": \"ANI6+wXQBUVDUFFVdzBBR1VQRkMuBApERW4nS1FTRkgvKTEAGTwQeEtZREJUdkdVV3tGCWl3akZOXFg4G3Q2Ymk7cUFYWygGCBQqDgZVSUUzPW5LaUUxRlUpImJcRkFkMRscJyQ6RlhxRTxODAtKNTE3MGRlRDFdVGZqVDc1fScXNH5IX1AlCzkjKRdRNUVbXGYMLz0/JigQBg5gVmNiW3oxeUtxCFU6I1J6WDUyXiEyGhlSQz0/QxgpJzIqXyxXV35eaDBRJ059aHAVPhk5P2N6LzoeVls=\""
"}";

int main(int argc, char** argv) {
	UltAlprSdkResult result;
	std::string charset = "latin";
	std::string jsonConfig = __jsonConfig;
	
	ULTALPR_SDK_PRINT_INFO("Starting recognizer...");
	result = UltAlprSdkEngine::init(jsonConfig.c_str());


    cv::Mat frame;
    cv::VideoCapture cap(argv[1]);
    if (!cap.isOpened()) {
        std::cerr << "ERROR! Unable to open.\n";
        return -1;
    }
    std::cout << "Start grabbing" << std::endl
        << "Press any key to terminate" << std::endl;


	
	int numRepeat = 7, refresh = 10000, residual = 200;
	int numIter = 0;
	std::vector<std::string> allPrevDigits;
	std::fstream registered;
	registered.open("../registered.txt", std::ios::app);


    while (true) {

		if (numIter++ > refresh) {
			numIter = 0;
			allPrevDigits = std::vector<std::string>(allPrevDigits.end()-residual, allPrevDigits.end());
		}

        cap.read(frame);
        if (frame.empty()) {
            std::cerr << "ERROR! blank frame grabbed\n";
            break;
        }
		//recognize
		result = UltAlprSdkEngine::process(
			ULTALPR_SDK_IMAGE_TYPE_BGR24,
			frame.data,
			1280,
			720
		);

		// Print latest result
		if (result.numPlates()) {
			const std::string& json_ = result.json();
			if (!json_.empty()) {
				nlohmann::json parsed = nlohmann::json::parse(json_);
				std::cout << parsed << std::endl;
				for (int i = 0; i < result.numPlates(); i++) {
					std::string digits = parsed.at("plates").at(i).at("text").get<std::string>();
					const std::vector<double> loc = parsed.at("plates").at(i).at("warpedBox").get<std::vector<double> >();
					if (digits.length() == 6 || digits.length() == 7) {
						std::replace(digits.begin(), digits.end(), 'I', '1'); // Taiwanese standard
						std::replace(digits.begin(), digits.end(), 'O', '0'); // Taiwanese standard
						std::replace(digits.begin(), digits.end(), 'W', 'M'); // ambiguous and M is far more than W
	
						allPrevDigits.push_back(digits);
						if (std::count(allPrevDigits.begin(), allPrevDigits.end(), digits) == numRepeat) {
							registered << digits << std::endl;
						}

						cv::putText(
							frame,
							digits,
							cv::Point(loc[0]-20, loc[1]-20),
							cv::FONT_HERSHEY_DUPLEX,
							1.0,
							cv::Scalar(255, 255, 255),
							2
						);
						cv::rectangle(
							frame,
							cv::Point(loc[0], loc[1]),
							cv::Point(loc[4], loc[5]),
							cv::Scalar(0, 255, 0),
							2,
							cv::LINE_AA
						);
					}
				}
			}
		}

        // show live and wait for a key with timeout long enough to show images
        cv::imshow("Live", frame);
        if (cv::waitKey(5) >= 0)
            break;
    }
	registered.close();
    // the camera will be deinitialized automatically in VideoCapture destructor
	// DeInit
		ULTALPR_SDK_PRINT_INFO("Ending recognizer...");
		result = UltAlprSdkEngine::deInit();
    return 0;
}
