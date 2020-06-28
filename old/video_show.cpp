#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>
#include <stdlib.h>
#include "yuv_to_jpeg.h"
#include "unistd.h"
#include <opencv2/opencv.hpp> //opencv的头文件
#include "shm.h"

using namespace cv; //使用命名空间cv,使用过C++的都明白，我们写C++程序必须使用using namespace std。

int image_width=1280;
int image_height=720;

char buffer[100 * 1024];
unsigned int  revLen = 0;

int main(int argc, char **argv)
{
	void *shm = NULL;
	

	//申请共享内存
	shm = shmInit(100 * 1024);

	while (1)
	{
		/*从共享内存读jpg 图片 */
		shmRead((char *)buffer, &revLen, (char *)shm);

		CvMat mCvmat = cvMat(image_width, image_height, CV_8UC1, buffer);
		IplImage *IpImg = cvDecodeImage(&mCvmat, 1);
		//opencv3.0 IplImage到Mat类型的转换的方法
		Mat image = cvarrToMat(IpImg);
		if (!image.data)
			return false;
		cvReleaseImage(&IpImg);
		imshow("显示灰度图", image);
		waitKey(10);
	}

	//断开共享内存
	shmDetach();
	return 0;
}
