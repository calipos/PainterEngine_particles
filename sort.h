#ifndef _SORT_H
#define _SORT_H

#ifdef __cplusplus
extern "C" {
#endif 

	void sort(
		int* number,
		float* positions,
		float* diameter,
		unsigned int* ballHash,
		unsigned int* ballIndex);
	void sortRender(
		int* number,
		float* distance, 
		unsigned int* ballIndex);

#ifdef __cplusplus
}
#endif



#endif // !_SORT_H
