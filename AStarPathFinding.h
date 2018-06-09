#pragma once
#ifndef A_STAR_PATH_FINDING_H_
#define A_STAR_PATH_FINDING_H_

#include <GridMap.h>
#include "cocos2d.h"
#include <vector>

class Grid;

typedef std::vector<std::vector<Grid>> grid_matrix;
typedef std::vector<Grid *> grid_vec;
enum { VACANT, OCCUPIED, START, DESTINATION, OPEN, CLOSE };

class Grid {
public:
	Grid();
	int getFlag()const { return _f; }
	void setFlag(int flag) { _flag = flag; }
	int getX()const { return _x; }
	int getY()const { return _y; }
	void setPosition(int x, int y) { _x = x; _y = y; }
	int getG()const { return _g; }
	void setG(int g) { _g = g; }
	int getH()const { return _h; }
	void setH(int h) { _h = h; }
	int getF()const { return _f; }
	void setF(int f) { _f = f; }
	Grid * getParent()const { return _parent; }
	void setParent(Grid* parent) { _parent = parent; }
private:
	int _x;
	int _y;
	int _flag;//����ı�־
	int _g;//Gֵ��ŷ����þ���(����㵽ָ��������ƶ�����)
	int _h;//Hֵ�������پ���(�Ӹõ㵽�յ�Ĺ�����ۣ���������)
	int _f;//���ߴ��� f = g + h
	Grid* _parent;//���ڵ�
};

class PathFinding {
public:
	PathFinding(dyadic_array& map, cocos2d::Point start, cocos2d::Point destination);
	//Ѱ��·��
	void searchForPath();
	//����·��
	void generatePath();
	GridPath getPath()const { return _path; }
private:
	int _width;									//��ͼ���
	int _height;								//��ͼ�߶�
	std::vector<std::vector<Grid>>  _map;		//��ͼ����
	Grid * _starting_point;						//��ʼ��
	Grid * _terminal_point;						//�յ�

	grid_vec _open_list;						//�����б�
	grid_vec _close_list;						//����б�
	GridPath _path;								//����·��

	//�ӵ�ǰ�����б���ѡȡ���ߴ���F��С�ĵĸ����Ϊ��һ������ĸ��
	Grid * getNextGrid();

	//��鵱ǰ�����Χ�˸�����״̬
	void checkSurroundedGrid(Grid & grid);

	//�жϵ�ǰ����Ƿ��ڵ�ͼ��Χ��
	bool isInMapRange(cocos2d::Point & grid);

	//�ж��Ƿ�Ϊת�ǣ�����б�Ŵ����ϰ���(��������������ƶ�����������)
	bool isCorner(Grid & g1, Grid & g2);

	//�жϸø���Ƿ����
	bool isAvailable(Grid & grid);

	//����ŷ����þ���(��Gֵ)
	int getEuclideanDistance(Grid & g1, Grid & g2);

	//���������پ���(��Hֵ)
	int getManhattanDistance(Grid & g1, Grid & g2);

	//�ӿ����б����Ƴ��Ѽ������б�ĸ��
	void removeFromOpenList(Grid * grid);
};


#endif