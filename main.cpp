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

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
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

// Sound
char MPLAYER_CTRL[] = "/tmp/mplayer-control";
int startMPlayerInBackground()
{
    pid_t processId = fork();
    if (processId == 0) {
        printf("running mplayer\n");
        char cmd[256];
        snprintf(cmd, 256, "mplayer -quiet -fs -slave -idle -input file=%s", MPLAYER_CTRL);
        int status = system(cmd);
        printf("mplayer ended with status %d\n", status);
        exit(status);
    } else {
        return processId;
    }
}

void send(char* cmd)
{
    int fdes = open(MPLAYER_CTRL, O_WRONLY);
    ssize_t dummy = write(fdes, cmd, strlen(cmd));
    close(fdes);
}


int main(int argc, char** argv) {
	// sound player init
	unlink(MPLAYER_CTRL);
    int res = mknod(MPLAYER_CTRL, S_IFIFO|0777, 0);
	pid_t processId = startMPlayerInBackground();
	if (processId < 0) {
        printf("failed to start child process\n");
		exit(1);
    }

	UltAlprSdkResult result;
	std::string charset = "latin";
	std::string jsonConfig = __jsonConfig;
	
	ULTALPR_SDK_PRINT_INFO("Starting recognizer...");
	result = UltAlprSdkEngine::init(jsonConfig.c_str());


    cv::VideoCapture cap(argv[1]);
    if (!cap.isOpened()) {
        std::cerr << "ERROR! Unable to open.\n";
        return -1;
    }
    std::cout << "Start grabbing" << std::endl
        << "Press any key to terminate" << std::endl;
	cv::VideoWriter video("out.mp4", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 30, cv::Size(1280, 720));

	int numRepeat = 5, refresh = 10000, residual = 200;
	int numIter = 0;
	std::vector<std::string> allPrevDigits;

	std::vector<std::string> registeredDigits;
	std::fstream registered;
	registered.open("../registered.txt", std::ios::in);
	std::string buff;
	while(registered >> buff)
		registeredDigits.push_back(buff);
	registered.close();
	double alpha = 0;
	double scale = std::atof(argv[2]);

    while (true) {
   		cv::Mat frame;
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

		bool warning = false;
		// Print latest result
		if (result.numPlates()) {
			const std::string& json_ = result.json();
			if (!json_.empty()) {
				nlohmann::json parsed = nlohmann::json::parse(json_);
				// std::cout << parsed << std::endl;
				for (int i = 0; i < result.numPlates(); i++) {
					std::string digits = parsed.at("plates").at(i).at("text").get<std::string>();
					const std::vector<double> loc = parsed.at("plates").at(i).at("warpedBox").get<std::vector<double> >();
					if (digits.length() == 6 || digits.length() == 7) {
						std::replace(digits.begin(), digits.end(), 'I', '1'); // Taiwanese standard
						std::replace(digits.begin(), digits.end(), 'O', '0'); // Taiwanese standard
						std::replace(digits.begin(), digits.end(), 'W', 'M'); // ambiguous and M is far more than W
	
						allPrevDigits.push_back(digits);
						if (std::count(allPrevDigits.begin(), allPrevDigits.end(), digits) == numRepeat) {
							if (std::count(registeredDigits.begin(), registeredDigits.end(), digits) != 0)
								warning = true;
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
							2
						);
						
					}
				}

			}
		}

		if (warning) {
			char* soundPath = (char*)"loadfile ../sound/sound.mp3\n";
			send(soundPath);
			alpha = 0.8;
		}
		alpha-=0.025;
		if (alpha < 0)
			alpha = 0;
		if (alpha != 0) {
			cv::Mat overlay;
			frame.copyTo(overlay);
			cv::rectangle(
				overlay,
				cv::Point(0, 0),
				cv::Point(1280, 720),
				cv::Scalar(0, 0, 255),
				-1
			);
			cv::addWeighted(overlay, alpha, frame, 1- alpha, 0, frame);
		}

		cv::resize(
			frame,
			frame,
			cv::Size(frame.cols*scale, frame.rows*scale)
		);
		

        // show live and wait for a key with timeout long enough to show images
        cv::imshow("Live", frame);
		video.write(frame);
        if (cv::waitKey(5) >= 0)
            break;
    }
	cap.release();
	video.release();
	// DeInit
		ULTALPR_SDK_PRINT_INFO("Ending recognizer...");
		result = UltAlprSdkEngine::deInit();
    return 0;
}
