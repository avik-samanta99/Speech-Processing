/*
=============================
Name : Avik Samanta
Roll no. - 204101016
Asgn. 1 - YES/NO detection
Submission Date : 25/09/2020
=============================
*/

#include "stdafx.h"
#include "stdio.h"
#include "stdlib.h"
#include "vector"
#include "iostream"
using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	/*
	=============================
	Reading the voice information
	=============================
	*/

	// "inPtr" : File pointer to read the file we want to work with
	FILE *inPtr;

	// Opening the file in "read" mode, as we require the intensity values of the voice signal
	fopen_s(&inPtr, "Voices/yes_no4.txt", "r");

	// If there is no such file, we will exit the program
	if(!inPtr){
		cout << "Can't open the file\n";
		exit(0);
	}

	// "intensity" : The vactor to store the intensity values 
	vector<long long> intensity;

	// We will pick the intensity values from the file, and store them into the vector "intensity"
	long long curIntensity;
	fscanf_s(inPtr, "%lld", &curIntensity);
	while(!feof(inPtr)){ // feof(filePtr) : denotes that the pointer reached the End of The File (EOF) character
		intensity.push_back(curIntensity);
		fscanf_s(inPtr, "%lld", &curIntensity);
	}

	/*
	==========================================================
	Calculating the ZCR and STE values (with frame size : 320)
	==========================================================
	*/

	// "ste" : It stores the Short Term Energy (STE) values, for each frame of size 320
	// "zcr" : It stores the Zero Crossing Rate (ZCR) values, for each frame of size 320
	// We are calculating STE values like, "mean of squared intensity values" in each disjoint window of size 320
	// We are calculating ZCR values like, "No of times the signal crossed/touches the x axis" in each disjoint window of size 320
	vector<long long> ste, zcr;
	long long frameSize = 320, curFrame = 0, cur_ste = 0, cur_zcr = 0;
	for(unsigned int i = 0; i < intensity.size(); i ++){
		curFrame ++;
		cur_ste += (intensity[i] * intensity[i]);
		if(i > 0 && (intensity[i-1] * intensity[i] <= 0)) cur_zcr ++;
		if(curFrame == frameSize){
			cur_ste /= frameSize;
			ste.push_back(cur_ste);
			zcr.push_back(cur_zcr);
			cur_ste = cur_zcr = 0;
			curFrame = 0;
		}
	}

	/*
	===================================
	Detecting the sound/word boundaries
	===================================
	*/

	// "start_boundary" & "end_boundary" : These vectors keep track of the markers which denote the boundary of the a sound
	// "start_boundary" keeps track of the starting point, and "end_boundary" keeps track of the end point
	vector<long long> start_boundary, end_boundary;

	// "tot_ste" : Temporary variable to keep track of the total STE value of the current sound (YES / NO / Silence)
	// "avg_ste" : Temporary variable to keep track of the average STE value of the current sound (YES / NO / Silence)
    // "tot_zcr" : Temporary variable to keep track of the total ZCR value of the current sound (YES / NO / Silence)
	// "avg_zcr" : Temporary variable to keep track of the average ZCR value of the current sound (YES / NO / Silence)
	// "last_boundary" : This keeps track of where we had put our last boundary marker 
	// To calculate the boundary, I have used some "threshold values" (for ZCR) and "% changes" (in STE)
	long long tot_ste = 0, tot_zcr = 0, avg_ste = 0, avg_zcr = 0;
	long long last_boundary = -1;
	for(unsigned int i = 0; i < ste.size(); i ++){
		tot_ste += ste[i]; 
		tot_zcr += zcr[i];
		avg_ste = tot_ste / (i-last_boundary+1);
		avg_zcr = tot_zcr / (i-last_boundary+1);
		if(i > 1){
			if(ste[i] >= avg_ste * 5 && zcr[i] < 50 && avg_zcr > 30){
				start_boundary.push_back((long long)i);
				last_boundary = (long long)i;
				tot_ste = ste[i];
				tot_zcr = zcr[i];
			}
			else if(ste[i] < avg_ste / 150 && abs(zcr[i] - avg_zcr) <= 60){
				end_boundary.push_back((long long)(i-1));
				last_boundary = (long long)i;
				tot_ste = ste[i];
				tot_zcr = zcr[i];
			}
			else if(ste[i] < avg_ste / 150 && zcr[i] >= 40 && avg_zcr < 40){
				end_boundary.push_back((long long)(i-1));
				last_boundary = (long long)i;
				tot_ste = ste[i];
				tot_zcr = zcr[i];
			}
		}
	}

	/*
	======================================
	Distinguishing the word (if YES or NO)
	======================================s
	*/

	// Now as we have the boundary described for ourselves, we will distinguish the uttered word
	// I will distinguish based on the ZCR values
	// Due to the presence of "s" Fricative in the word "YES", the ZCR values at the end section of word "YES" are very high, resulting in a high average ZCR for the word
	// As the word "NO" consists of a nasal consonant "n" and vowel "o", both of them have very low ZCR values, resulting in a low average ZCR for the word
	// For my voice data, the "NO" have average ZCR strictly less than "50", where as for "YES" it is greater than "50", may even go more than "100"
	cout << "Here is the result for our implementation of YES/NO detection :-\n\n";
	for(unsigned int i = 0; i < start_boundary.size(); i ++){
		tot_zcr = 0;
		for(unsigned int j = (unsigned int)start_boundary[i]; j <= (unsigned int)end_boundary[i]; j ++){
			tot_zcr += zcr[j];
		}
		avg_zcr = tot_zcr / (end_boundary[i]-start_boundary[i]+1);
		if(avg_zcr < 50) cout << "NO  (starting from frame : " << start_boundary[i] << ", ending at frame : " << end_boundary[i] << ", having avg. ZCR = " << avg_zcr << " [ < 50])\n";
		else if(avg_zcr >= 50) cout << "YES (starting from frame : " << start_boundary[i] << ", ending at frame : " << end_boundary[i] << ", having avg. ZCR = " << avg_zcr << " [ >= 50])\n";
	} cout << endl;

	return 0;
}

