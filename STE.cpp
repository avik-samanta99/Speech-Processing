// STE.cpp : Defines the entry point for the console application.
//

#include <stdafx.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	FILE *inPtr1, *inPtr2, *inPtr3, *outPtr;
	fopen_s(&inPtr1, "C:\\Users\\hp\\Desktop\\Speech Processing - PKD\\Cool Edit\\Silence_32087.txt", "r");
	fopen_s(&inPtr2, "C:\\Users\\hp\\Desktop\\Speech Processing - PKD\\Cool Edit\\vowel_aa_32078.txt", "r");
	fopen_s(&inPtr3, "C:\\Users\\hp\\Desktop\\Speech Processing - PKD\\Cool Edit\\fricative_sh_32073.txt", "r");
	fopen_s(&outPtr, "C:\\Users\\hp\\Desktop\\Speech Processing - PKD\\Cool Edit\\Out.txt", "a");
	if(!inPtr1 || !inPtr2 || !inPtr3){
		cout << "File reading failed!! Exiting...\n";
		exit(0);
	}
	if(!outPtr){
		cout << "File reading failed!! Exiting...\n";
		exit(0);
	}

	long long chunk = 320;
	vector<long long> values;
	long long curVal;

	fprintf_s(outPtr, "For silence :-\n");
	fscanf_s(inPtr1, "%lld", &curVal);
	while(!feof(inPtr1)){
		values.push_back(curVal);
		fscanf_s(inPtr1, "%lld", &curVal);
	}
	long long groups = values.size() / chunk;
	long long j, sum;
	for(long long i = 0; i < groups; i ++){
		sum = 0;
		long long start = i * chunk, end = (i+1) * chunk;
		for(j = start; j < end; j ++) sum += (values[(unsigned int)j]*values[(unsigned int)j]);
		sum /= chunk;
		fprintf_s(outPtr, "%lld ", sum);
	}
	fprintf_s(outPtr, "\n");

	values.clear();
	fprintf_s(outPtr, "For vowel(voiced) : \"aa\" :-\n");
	fscanf_s(inPtr2, "%lld", &curVal);
	while(!feof(inPtr2)){
		values.push_back(curVal);
		fscanf_s(inPtr2, "%lld", &curVal);
	}
	groups = values.size() / chunk;
	for(long long i = 0; i < groups; i ++){
		sum = 0;
		long long start = i * chunk, end = (i+1) * chunk;
		for(j = start; j < end; j ++) sum += (values[(unsigned int)j]*values[(unsigned int)j]);
		sum /= chunk;
		fprintf_s(outPtr, "%lld ", sum);
	}
	fprintf_s(outPtr, "\n");

	values.clear();
	fprintf_s(outPtr, "For fricative(unvoiced) : \"sh\" :-\n");
	fscanf_s(inPtr3, "%lld", &curVal);
	while(!feof(inPtr3)){
		values.push_back(curVal);
		fscanf_s(inPtr3, "%lld", &curVal);
	}
	groups = values.size() / chunk;
	for(long long i = 0; i < groups; i ++){
		sum = 0;
		long long start = i * chunk, end = (i+1) * chunk;
		for(j = start; j < end; j ++) sum += (values[(unsigned int)j]*values[(unsigned int)j]);
		sum /= chunk;
		fprintf_s(outPtr, "%lld ", sum);
	}
	fprintf_s(outPtr, "\n");

	fclose(inPtr1);
	fclose(inPtr2);
	fclose(inPtr3);
	fclose(outPtr);
	system("pause");
	return 0;
}

