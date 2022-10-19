#include "sort.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <map>
#include <list>
#include <string>

void sort(
	int* number,
	float* positions,
	float* diameter,
	unsigned int* ballHash,
	unsigned int* ballIndex)
{
	std::vector<std::pair<unsigned int, unsigned int>>hash_idx(*number);
	for (int index = 0; index < *number; index++)
	{
		float* p_x = &positions[3 * index];
		float* p_y = p_x + 1;
		float* p_z = p_x + 2;
		int gridPos_x = std::floor((*p_x +.5) / *diameter);
		int gridPos_y = std::floor((*p_y +.5) / *diameter);
		int gridPos_z = std::floor((*p_z) / *diameter);
		gridPos_x = gridPos_x & 63;
		gridPos_y = gridPos_y & 63;
		gridPos_z = gridPos_z & 63;
		unsigned int hash = gridPos_z * (4096) + (gridPos_y * 64) + gridPos_x;
		hash_idx[index].first = hash;
		hash_idx[index].second = index;
	}
	std::sort(hash_idx.begin(), hash_idx.end(), [](const auto& a, const auto& b) {return a.first < b.first; });
	int temp = 0;
	for (auto& d : hash_idx)
	{
		ballHash[temp] = d.first;
		ballIndex[temp] = d.second;
		temp++;
	}
	return;
}

void sortRender(
	int* number,
	float* distance,
	unsigned int* ballIndex)
{
	std::vector<std::pair<float, unsigned int>>dist_idx(*number);
	for (int index = 0; index < *number; index++)
	{ 
		dist_idx[index].first = distance[index];
		dist_idx[index].second = index;
	}
	std::sort(dist_idx.begin(), dist_idx.end(), [](const auto& a, const auto& b) {return a.first> b.first; });
	int temp = 0;
	for (auto& d : dist_idx)
	{ 
		ballIndex[temp] = d.second;
		temp++;
	}
	return;
}