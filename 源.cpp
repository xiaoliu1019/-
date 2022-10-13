#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/types_c.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include<vector>
#include<math.h>
#include<algorithm>

using namespace cv;
using namespace std;

class Seat
{
public:
	Seat(Rect rect, float area, int color_index,int count) {
		this->rect = rect;
		this->area = area;
		this->color_index = color_index;
		this->count = count;
	}
	int count;
	float area;
	Rect rect;
	int color_index;
	int row = 0;
	int col;
	bool falg = 0;
};

bool seat_compare_x(const Seat &A, const Seat &B) {
	return A.rect.tl().x > B.rect.tl().x;
}

bool seat_compare_y(const Seat &A, const Seat &B) {
	return A.rect.tl().y > B.rect.tl().y;
}

bool seat_compare_count(const Seat &A, const Seat &B) {
	return A.count < B.count;
}

vector<vector <int>>base_color = vector<vector <int>>(6,vector<int>(3)) = {
		{255, 204, 204},	//��ɫ
		{67, 163, 255},		//��ɫ
		{94, 111, 254},		//��ɫ
		{203, 192, 255},  //��ɫ
		{ 226, 228, 229 }, //��ɫ
		{ 50, 176, 102 }    //��ɫ
	};

string base_color_name[6] = { "��ɫ", "��ɫ", "��ɫ", "��ɫ", "��ɫ", "��ɫ" };

//���Ҿ��ο����ɫ
int find_color(const Mat rect_img) {
	int color_cnt[6] = { 0 };
	for (int i = 0; i < rect_img.rows; i++) {
		for (int j = 0; j < rect_img.cols; j++) {
			Vec3b pixel = rect_img.at<Vec3b>(i, j);
			for (int c = 0; c < 6; c++) {
				double dis = sqrt(pow(pixel[0] - base_color[c][0], 2) +
					pow(pixel[1] - base_color[c][1], 2) +
					pow(pixel[2] - base_color[c][2], 2));
				if (dis < 15) {
					color_cnt[c]++;
				}
			}
		}
	}
	int max = color_cnt[0];
	int index = 0;
	for (int i = 1; i < 6; i++) {
		if (color_cnt[i] > max) {
			max = color_cnt[i];
			index = i;
		}
	}
	//���ؾ��ο����ɫ�±�
	return index;
}

int main(){
	//����ͼƬ��ͼƬ���ڳ�������Ŀ¼
	//imread����ͼƬΪ bgrͼ��
	Mat img_bgr = imread("./cut.png");

	//bgrת��Ϊrgbͼ��
	Mat img_rgb;
	cvtColor(img_bgr, img_rgb, COLOR_BGR2RGB);

	//ת��Ϊ�Ҷ�ͼ
	Mat gray;
	cvtColor(img_bgr, gray, COLOR_BGR2GRAY);
	imshow("gray", gray);

	//ʹ�þֲ���ֵ������Ӧ��ֵ��������ͼ���ֵ��
	//grayΪ����������ͼ��dstΪthreshold���������ͼ��, 0 Ϊ��ֵ������ֵ�� 255Ϊ���ֵ��THRESH_OTSUȷ�����ɶ�ֵ��ͼ����㷨
	Mat dst;
	threshold(gray, dst, 0, 255, THRESH_OTSU);
	imshow("dst1", dst);

	//��̬ѧȥ��, getStructuringElement()��������ָ����״�ͳߴ�ĽṹԪ�أ��ں˾���) (3*3��С��ʮ�־���ṹԪ��)
	Mat element = getStructuringElement(MORPH_CROSS, Size(3, 3));

	//������ȥ�� ����������ȸ�ʴ�����ͣ��������ڽ��룬����һЩС�����ص�
	morphologyEx(dst, dst, MORPH_OPEN, element);
	imshow("dst2", dst);

	//������⺯��,��һ�������������ͼ��Ҫ��ͼ��Ϊ��ͨ��ͼ�����Ϊ��ֵͼ�� ���ڶ��������������������Ϊvector<vector<Point>>����
	vector<vector<Point>> contours;

	/*������������ÿ��������Ӧ������;����Ϊ vector<Vec4i>hierarchy�е�ÿ��Ԫ�ض���һ����4��int����ɵļ��ϡ�ֱ�۵ı�ʾ���Բο�����Ϊ4������Ϊn�Ķ�ά����
	���ĸ�int����hierarchy[i][0]~hierarchy[i][3]�ֱ��ʾ��һ��������ǰһ������������������Ƕ������������ţ������ǰ��������Ӧ�����ĸ�����֮һ��ȱʧ��
	����˵�����ڵĵ�һ������Ϊû��ǰһ������������Ӧλ��hierarchy[i][1]=-1��*/
	vector<Vec4i>hierarchy; //Vec4i ��ʾΪ��ͨ��ͼ��  ��int[4];
	
	//���ĸ�������������ģʽ������һ���ȼ����ṹ������������������������Ľ��Ʒ�����
	findContours(dst, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

	//��������
	drawContours(dst, contours, -1, (120, 0, 0), 2);


	int count = 0;						//����
	double area_avrg = 0;				//ƽ��
	int	seat_cnt[6] = { 0 };		   //����ɫͳ��
	int	double_seat = 0;				//˫����

	//������λ����
	vector<Seat> seats;

	//������������
	for (int i = 0; i < contours.size(); i++) {
		double area = contourArea(contours[i]);
		if (area < 50 or area > 2000)
			continue;
		count++;
		area_avrg += area;

		if (area > 1000) {
			//��ȡ�������꣨x,y���������������ƾ��α߿�
			//��˫�������в��
			Rect rect = boundingRect(contours[i]);
			Rect rectl = Rect(rect.tl(), Size(rect.width / 2, rect.height));
			Rect rectr = Rect(rect.tl().x + rect.width / 2, rect.tl().y , rect.width / 2, rect.height);

			Mat maskl = dst(rectl);
			Mat maskr = dst(rectr);

			//������
			Mat rect_bgrl = img_bgr(rectl);

			int color_idx = find_color(rect_bgrl);
			seat_cnt[color_idx] += 2;

			//�Ҳ���� 
			Mat rect_bgrr = img_bgr(rectr);

			double_seat += 1;

			//����λ��ʵ����������뵽������
			seats.push_back(Seat(rectr, area / 2, color_idx, count));
			seats.push_back(Seat(rectl, area / 2, color_idx, ++count));

			//���ƾ���
			rectangle(img_bgr, rectr, (0, 0, 255), 1);
			rectangle(img_bgr, rectl, (0, 0, 255), 1);

			// �����Ͻ�д�ϱ��
			putText(img_bgr, to_string(count-1), rectr.tl(), FONT_HERSHEY_COMPLEX, 0.4, (0, 0, 0), 1);
			putText(img_bgr, to_string(count), rectl.tl(), FONT_HERSHEY_COMPLEX, 0.4, (0, 0, 0), 1);
		}
		else
		{
			//��ȡ�������꣨x,y���������������ƾ��α߿�
			Rect rect = boundingRect(contours[i]);

			//rect.area();     //����rect����� 5000
			//rect.size();     //����rect�ĳߴ� [50 �� 100]
			//rect.tl();       //����rect�����϶�������� [100, 50]
			//rect.br();       //����rect�����¶�������� [150, 150]
			//rect.width();    //����rect�Ŀ�� 50
			//rect.height();   //����rect�ĸ߶� 100
			//rect.contains(Point(x, y));  //���ز����������ж�rect�Ƿ����Point(x, y)��
			//��ȡmask,��ֵͼ�ľ��α߿��ڵ����ؿ�,��Rect����ȡͼƬ
			Mat mask = dst(rect);

			Mat rect_bgr = img_bgr(rect);

			int color_idx = find_color(rect_bgr);
			seat_cnt[color_idx] += 1;

			//����λ��ʵ����������뵽������
			seats.push_back(Seat(rect, area, color_idx, count));

			//���ƾ���
			rectangle(img_bgr, rect, (0, 0, 255), 1);

			//��ֹ��ŵ�ͼƬ֮�⣨���棩,��Ϊ���Ʊ��д�����Ͻǣ��������������yС��10�ı�Ϊ10������
			int y;
			if (rect.tl().y < 10)
				y = 10;
			else
			{
				y = rect.tl().y;
			}
			// �����Ͻ�д�ϱ��
			putText(img_bgr, to_string(count), rect.tl(), FONT_HERSHEY_COMPLEX, 0.4, (0, 0, 0), 1);
		}
	}
	
	//��y�����������
	sort(seats.begin(), seats.end(), seat_compare_y);

	int row = 1;
	for (int i = 0; i < seats.size(); i++) {
		if (i > 0 && seats[i - 1].rect.tl().y - seats[i].rect.tl().y > 20)
			row++;
		seats[i].row = row;
	}

	//��x�����������
	sort(seats.begin(), seats.end(), seat_compare_x);

	int col = 1;
	for (int i = 0; i < seats.size(); i++) {
		if (i > 0 && seats[i - 1].rect.tl().x - seats[i].rect.tl().x > 20)
			col++;
		seats[i].col = col;
	}
	
	sort(seats.begin(), seats.end(), seat_compare_count);

	cout << "��λ������" << count << "\t" << "�����Ϊ��" << area_avrg << endl;
	cout << "����˫����λ��" << double_seat << "��" << endl;
	for (int i = 0; i < 6; i++) {
		cout << "��λ��ɫ��" << base_color_name[i] << "\t" << "������" << seat_cnt[i] << endl;
		for (int j = 0; j < seats.size(); j++) {
			if (seats[j].color_index == i) {
				cout << "\t" << "��" << seats[j].row << "��  "
					<< "��" << seats[j].col << "��  " << endl;
			}
		}
	}


	imshow("bgr", img_bgr);
	waitKey(0);
	return 0;
}


