#pragma once

#include <openslide-features.h>
#include <openslide.h>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>


namespace WSIAnno
{
	struct sizepair
	{
		int64_t w;
		int64_t h;

		const int64_t width() const
		{
			return w;
		}

		const int64_t height() const
		{
			return h;
		}

		const double operator/(const sizepair& b)
		{
			return (double)w / b.w;
		}

		const sizepair operator*(const double ratio)
		{
			return sizepair(w*ratio, h*ratio);
		}

		const sizepair operator/(const double ratio)
		{
			return sizepair(w/ratio, h/ratio);
		}

		const bool operator>(const sizepair& b)
		{
			if (w > b.w || h > b.h)
				return true;
			else
				return false;
		}

		const bool operator<(const sizepair& b)
		{
			if (w < b.w || h < b.h)
				return true;
			else
				return false;
		}


		sizepair(int64_t w, int64_t h):w(w),h(h)
		{}

		sizepair()
		{}
	};

	class OpenSlide
	{
	public:
		OpenSlide(std::string path);
		OpenSlide():isReady(false){}

		//Reuse
		int loadFile(std::string path);
		void closeFile();

		//Image pyramids
		const int32_t getLvCounts();
		const sizepair& getLvDims(const int32_t lv);
		const double getLvDownSample(const int32_t lv);
		const int32_t getBestLv(const double downSample);
		const cv::Mat& readRegion(const cv::Point xy, const int32_t lv, const sizepair size);
		const cv::Mat& readWholeLv(const int32_t lv);

		//Properties
		const std::vector<std::string>& getPropNames();
		const std::string getPropVal(const std::string propName);

		//Associated images

		const std::vector<std::string>& getAssoImgNames();
		const sizepair& getAssoImgDims(const std::string assoName);
		const cv::Mat& readAssoImg(const std::string assoName);

		//Utility22
		static const std::string& getOSVer();
		const std::string& getLastErr();

		~OpenSlide();

	private:

		OpenSlide(OpenSlide&);

		openslide_t* imageset;

		int32_t levelCt;
		sizepair lv0Size;
		sizepair lastSize;

		uint32_t *temp;
		cv::Mat pixData;

		std::vector<std::string> propNames;
		std::vector<std::string> assoImgNames;

		static const std::string osver;
		std::string errStr;

		bool isReady;

	};
}

