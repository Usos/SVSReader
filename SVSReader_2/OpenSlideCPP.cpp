#include "OpenSlideCPP.h"


namespace WSIAnno
{
	const std::string OpenSlide::osver = openslide_get_version();

	OpenSlide::OpenSlide(std::string path):imageset(openslide_open(path.c_str())),temp(NULL),isReady(true)
	{
		levelCt = openslide_get_level_count(imageset);
		openslide_get_level0_dimensions(imageset, &lv0Size.w, &lv0Size.h);
		lastSize = lv0Size;
		//temp = new uint32_t[lv0Size.w*lv0Size.h];   //need delete in destruct

		const char* const* templines = openslide_get_property_names(imageset);
		std::string tempstr;
		int count(1);

		if (templines != NULL)
		{
			tempstr = *templines;
			while (!tempstr.empty())
			{
				propNames.push_back(tempstr);
				if (*(templines + count) != NULL)
					tempstr = *(templines + count);
				else
					break;
				count++;
			}
		}

		count = 1;
		templines = openslide_get_associated_image_names(imageset);
		if (templines != NULL)
		{
			tempstr = *templines;
			while (!tempstr.empty())
			{
				assoImgNames.push_back(tempstr);
				if (*(templines + count) != NULL)
					tempstr = *(templines + count);
				else
					break;
				count++;
			}
		}


		
	}

	int OpenSlide::loadFile(std::string path)
	{
		if (isReady)
			closeFile();

		imageset = openslide_open(path.c_str());
		if (NULL == imageset)
			return -1;
		temp = NULL;

		levelCt = openslide_get_level_count(imageset);
		openslide_get_level0_dimensions(imageset, &lv0Size.w, &lv0Size.h);
		lastSize = lv0Size;
		//temp = new uint32_t[lv0Size.w*lv0Size.h];   //need delete in destruct

		const char* const* templines = openslide_get_property_names(imageset);
		std::string tempstr;
		int count(1);

		if (templines != NULL)
		{
			tempstr = *templines;
			while (!tempstr.empty())
			{
				propNames.push_back(tempstr);
				if (*(templines + count) != NULL)
					tempstr = *(templines + count);
				else
					break;
				count++;
			}
		}

		count = 1;
		templines = openslide_get_associated_image_names(imageset);
		if (templines != NULL)
		{
			tempstr = *templines;
			while (!tempstr.empty())
			{
				assoImgNames.push_back(tempstr);
				if (*(templines + count) != NULL)
					tempstr = *(templines + count);
				else
					break;
				count++;
			}
		}

		isReady = true;
		return 0;
	}

	void OpenSlide::closeFile()
	{
		isReady = false;
		openslide_close(imageset);
		propNames.clear();
		assoImgNames.clear();

		delete temp;
	}

	const int32_t OpenSlide::getLvCounts()
	{
		return openslide_get_level_count(imageset);
	}

	const sizepair & OpenSlide::getLvDims(const int32_t lv)
	{
		openslide_get_level_dimensions(imageset, lv, &lastSize.w, &lastSize.h);
		return lastSize;
	}

	const double OpenSlide::getLvDownSample(const int32_t lv)
	{
		return openslide_get_level_downsample(imageset,lv);
	}

	const int32_t OpenSlide::getBestLv(const double downSample)
	{
		return openslide_get_best_level_for_downsample(imageset,downSample);
	}

	const cv::Mat& OpenSlide::readRegion(const cv::Point xy, const int32_t lv, const sizepair size)
	{
		if (temp != NULL)
			delete temp;

		temp = new uint32_t[size.width()*size.height()];
		openslide_read_region(imageset, temp, xy.x, xy.y, lv, size.width(), size.height());
		pixData = cv::Mat::zeros(size.width(), size.height(), CV_8UC4);

		if (getLastErr().empty())
		{
			pixData = cv::Mat(size.height(), size.width(), CV_8UC4, temp).clone();
		}

		delete temp;
		temp = NULL;
	
		return pixData;
	}

	const cv::Mat & OpenSlide::readWholeLv(const int32_t lv)
	{
		getLvDims(lv);
		return readRegion(cv::Point(0,0), lv, lastSize);
	}

	const std::vector<std::string>& OpenSlide::getPropNames()
	{
		return propNames;
	}

	const std::string OpenSlide::getPropVal(const std::string propName)
	{
		return openslide_get_property_value(imageset, propName.c_str());
	}

	const std::vector<std::string>& OpenSlide::getAssoImgNames()
	{
		return assoImgNames;
	}

	const sizepair & OpenSlide::getAssoImgDims(const std::string assoName)
	{
		openslide_get_associated_image_dimensions(imageset, assoName.c_str(), &lastSize.w, &lastSize.h);
		return lastSize;
	}

	const cv::Mat& OpenSlide::readAssoImg(const std::string assoName)
	{
		if (temp != NULL)
			delete temp;

		getAssoImgDims(assoName);
		temp = new uint32_t[lastSize.h*lastSize.w];
		openslide_read_associated_image(imageset, assoName.c_str(), temp);

		pixData = cv::Mat::zeros(lastSize.h, lastSize.w, CV_8UC4);
		
		if (getLastErr().empty())
		{
			pixData = cv::Mat(lastSize.h, lastSize.w, CV_8UC4, temp).clone();
		}

		delete temp;
		temp = NULL;

		return pixData;
	}

	OpenSlide::~OpenSlide()
	{
		if (isReady)
		{
			openslide_close(imageset);
			delete temp;
		}
	}

	const std::string& OpenSlide::getOSVer()
	{
		return osver;
	}

	const std::string& OpenSlide::getLastErr()
	{
		if (openslide_get_error(imageset) != NULL)
			errStr = openslide_get_error(imageset);
		else
			errStr = std::string();

		return errStr;
	}


}