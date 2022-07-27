//Use the library for reading, generating jpg and video
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs/legacy/constants_c.h>

#include <fstream>

//Read a image and return its pixel matrix and size
cv::Mat readMat(std::string filename) {
	cv::Mat image;
	image = cv::imread(filename, CV_LOAD_IMAGE_COLOR);
	return image;
}
//image group to video
void MatsToVideo(std::vector<cv::Mat> mats, std::string videoName) {
	cv::VideoWriter writer;
	writer.open(videoName, writer.fourcc('M', 'J', 'P', 'G'), 10, cv::Size(mats[0].cols, mats[0].rows), true);
	for(size_t i = 0; i < mats.size(); i++) {
		writer.write(mats[i]);
	}
	writer.release();
}
//video to image group
std::vector<cv::Mat> videoToMats(std::string videoName) {
	cv::VideoCapture capture;
	capture.open(videoName);
	std::vector<cv::Mat> mats;
	cv::Mat				 frame;
	while(capture.read(frame)) {
		cv::Mat image;
		frame.convertTo(image, CV_8UC3);
		mats.push_back(image);
	}
	return mats;
}

//Adding up the rgb values on each pixel of two image
cv::Mat MatAddtract(cv::Mat image1, cv::Mat image2) {
	cv::Mat result;
	//for each pixel of image1 and image2, unsigned add the rgb values
	for(int i = 0; i < image1.rows; i++) {
		for(int j = 0; j < image1.cols; j++) {
			cv::Vec3b p1 = image1.at<cv::Vec3b>(i, j);
			cv::Vec3b p2 = image2.at<cv::Vec3b>(i, j);
			cv::Vec3b p3;
			p3[0]					   = p1[0] + p2[0];
			p3[1]					   = p1[1] + p2[1];
			p3[2]					   = p1[2] + p2[2];
			result.at<cv::Vec3b>(i, j) = p3;
		}
	}
	return result;
}
//Subtracting the rgb values on each pixel of two image
cv::Mat MatSubtract(cv::Mat image1, cv::Mat image2) {
	cv::Mat result;
	//for each pixel of image1 and image2, unsigned subtract the rgb values
	for(int i = 0; i < image1.rows; i++) {
		for(int j = 0; j < image1.cols; j++) {
			cv::Vec3b p1 = image1.at<cv::Vec3b>(i, j);
			cv::Vec3b p2 = image2.at<cv::Vec3b>(i, j);
			cv::Vec3b p3;
			p3[0]					   = p1[0] - p2[0];
			p3[1]					   = p1[1] - p2[1];
			p3[2]					   = p1[2] - p2[2];
			result.at<cv::Vec3b>(i, j) = p3;
		}
	}
	return result;
}
//Reads an arbitrary binary file and returns a image group based on a specific length and width
//The first four bytes of the first image are used to store the file size
std::vector<cv::Mat> readBinaryAsMatGroup(std::string filename, size_t length, size_t width) {
	std::vector<cv::Mat> mats;
	std::ifstream		 file(filename, std::ios::binary);
	if(file.is_open()) {
		//get the file size
		uint64_t fileSize		= 0;
		auto	 sizeOfFileSize = sizeof(fileSize);
		file.seekg(0, std::ios::end);
		fileSize = file.tellg();
		file.seekg(0, std::ios::beg);
		size_t EachImageSize = length * width * 3;
		char*  buffer		 = new char[EachImageSize];
		if(EachImageSize < sizeOfFileSize)
			return {};
		//write the first four bytes to store the file size
		memcpy(buffer, &fileSize, sizeOfFileSize);
		//read the rest of the file
		file.read(buffer + sizeOfFileSize, EachImageSize - sizeOfFileSize);
		//create first image
		cv::Mat image(cv::Size(width, length), CV_8UC3, buffer);
		mats.push_back(image.clone());
		//create the rest of the images
		uint64_t imageCount = (fileSize + sizeOfFileSize) / EachImageSize;
		if((fileSize + sizeOfFileSize) % EachImageSize)
			imageCount++;
		for(uint64_t i = 1; i < imageCount; i++) {
			file.read(buffer, EachImageSize);
			cv::Mat image(cv::Size(width, length), CV_8UC3, buffer);
			mats.push_back(image.clone());
		}
		delete[] buffer;
	}
	file.close();
	return mats;
}
//Write a image group to a binary file
void WriteMatGroupToBinary(std::vector<cv::Mat> mats, std::string filename) {
	std::ofstream file(filename, std::ios::binary);
	if(file.is_open()) {
		//get the file size
		uint64_t fileSize		= 0;
		auto	 sizeOfFileSize = sizeof(fileSize);
		memcpy(&fileSize, mats[0].data, sizeOfFileSize);
		//write the rest of the first image
		size_t EachImageSize = mats[0].cols * mats[0].rows * 3;
		file.write((const char*)mats[0].data + sizeOfFileSize, std::min<uint64_t>(EachImageSize - sizeOfFileSize, fileSize));
		fileSize -= std::min<uint64_t>(EachImageSize - sizeOfFileSize, fileSize);
		//write the rest of the file
		for(size_t i = 1; i < mats.size(); i++) {
			EachImageSize = mats[i].cols * mats[i].rows * 3;
			file.write((const char*)mats[i].data, std::min<uint64_t>(EachImageSize, fileSize));
			fileSize -= std::min<uint64_t>(EachImageSize, fileSize);
		}
	}
	file.close();
}

//now we can use the functions to convert any binary file to a video and vice versa!

//File to video
void binaryToVideo(std::string filename, std::string videoName, size_t width = 1920, size_t length = 1080) {
	std::vector<cv::Mat> mats = readBinaryAsMatGroup(filename, length, width);
	MatsToVideo(mats, videoName);
}
//Video to file
void videoToBinary(std::string videoName, std::string filename) {
	std::vector<cv::Mat> mats = videoToMats(videoName);
	WriteMatGroupToBinary(mats, filename);
}

//File and video conversion based on an original image
//The original image is stored as the first frame of the video
//The the content of each rest frame is saved by adding it to the original image
void binaryToVideoWithOriginalJpg(std::string filename, std::string videoName, std::string originalImageName) {
	cv::Mat				 originalImage = cv::imread(originalImageName);
	std::vector<cv::Mat> mats		   = readBinaryAsMatGroup(filename, originalImage.rows, originalImage.cols);
	cv::Mat&			 firstFrame	   = originalImage;
	for(size_t i = 0; i < mats.size(); i++) {
		mats[i] = MatAddtract(originalImage, mats[i]);
	}
	mats.insert(mats.begin(), firstFrame);
	MatsToVideo(mats, videoName);
}
//reverse of the above function
void videoToBinaryWithOriginalJpg(std::string videoName, std::string filename) {
	std::vector<cv::Mat> mats		   = videoToMats(videoName);
	cv::Mat				 originalImage = mats[0];
	mats.erase(mats.begin());
	for(size_t i = 0; i < mats.size(); i++) {
		mats[i] = MatSubtract(mats[i], originalImage);
	}
	WriteMatGroupToBinary(mats, filename);
}

//main function
int main(size_t _argc, char** _argv) {
	//_argv to vector<string>
	size_t					 argc = _argc;
	std::vector<std::string> argv;
	for(size_t i = 0; i < _argc; i++) {
		argv.push_back(_argv[i]);
	}

	{
		//if the first argument is -h or --help, print the help message
		if(argc <= 1 || argv[1] == "-h" || argv[1] == "--help") {
		print_help:
			std::cout << "Usage: " << argv[0] << " [option] [argument]" << std::endl;
			std::cout << "Options:" << std::endl;
			std::cout << "  -h, --help: print this help message" << std::endl;
			std::cout << "  -b, --binary: convert a video to a binary file" << std::endl;
			std::cout << "  -v, --video: convert a binary file to a video" << std::endl;
			std::cout << "  -b2, --binary2: convert a video to a binary file with an original image" << std::endl;
			std::cout << "  -v2, --video2: convert a binary file to a video with an original image" << std::endl;
			return 0;
		}
		else if(argv[1] == "-b" || argv[1] == "--binary") {
			//if the second argument is a file name, convert the file to a video
			if(argc == 3) {
				videoToBinary(argv[2], argv[2] + ".bin");
			}
			//if the second argument is a file name and a video name, convert the file to a video with the given video name
			else if(argc == 4) {
				videoToBinary(argv[2], argv[3]);
			}
			else {
				goto print_help;
			}
		}
		else if(argv[1] == "-v" || argv[1] == "--video") {
			//if the second argument is a file name, convert the file to a video
			if(argc == 3) {
				binaryToVideo(argv[2], argv[2] + ".avi");
			}
			//if the second argument is a file name and a video name, convert the file to a video with the given video name
			else if(argc == 4) {
				binaryToVideo(argv[2], argv[3]);
			}
			//if the second argument is a file name and a video name and a length and width, convert the file to a video with the given video name and length and width
			else if(argc == 6) {
				binaryToVideo(argv[2], argv[3], std::stoi(argv[4]), std::stoi(argv[5]));
			}
			else {
				goto print_help;
			}
		}
		else if(argv[1] == "-b2" || argv[1] == "--binary2") {
			//if the second argument is a file name, convert the file to a video
			if(argc == 4) {
				videoToBinaryWithOriginalJpg(argv[2], argv[2] + ".bin");
			}
			//if the second argument is a file name and a video name, convert the file to a video with the given video name
			else if(argc == 5) {
				videoToBinaryWithOriginalJpg(argv[2], argv[3]);
			}
			else {
				goto print_help;
			}
		}
		else if(argv[1] == "-v2" || argv[1] == "--video2") {
			//if the second argument is a file name, convert the file to a video
			if(argc == 4) {
				binaryToVideoWithOriginalJpg(argv[2], argv[2] + ".avi", argv[3]);
			}
			//if the second argument is a file name and a video name, convert the file to a video with the given video name
			else if(argc == 5) {
				binaryToVideoWithOriginalJpg(argv[2], argv[3], argv[4]);
			}
			else {
				goto print_help;
			}
		}
	}

	return 0;
}
