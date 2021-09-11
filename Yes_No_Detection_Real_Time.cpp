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
	===========================================
	Recording and Reading the voice information
	===========================================
	*/

	// Recording using the Recording-module (3 second clip)
	system("Recording_Module.exe 10 input_file.wav input_file.txt");

	// "inPtr" : File pointer to read the file we want to work with
	FILE *inPtr;

	// Opening the file in "read" mode, as we require the intensity values of the voice signal
	fopen_s(&inPtr, "input_file.txt", "r");

	// If there is no such file, we will exit the program
	if(!inPtr){
		cout << "Can't open the file\n";
		exit(0);
	}

	// "intensity" : The vactor to store the intensity values 
	vector<long double> intensity;

	// We will pick the intensity values from the file, and store them into the vector "intensity"
	long double curIntensity;
	fscanf_s(inPtr, "%Lf", &curIntensity);
	while(!feof(inPtr)){ // feof(filePtr) : denotes that the pointer reached the End of The File (EOF) character
		intensity.push_back(curIntensity);
		fscanf_s(inPtr, "%Lf", &curIntensity);
	}

	/*
	=============================================================================
	Data Preprocessing : Throw out initial chunk of data, Normalization
	=============================================================================
	*/

	// Throw out the initial disturbance [due to poor power supply/ voltage fluctuations/ old PC architecture/ wrongly onfigured microphone]
	// Let's throw out first 3 frames of data (or may be 1000 samples)
	intensity.erase(intensity.begin(), intensity.begin() + 1000);

	// Now we should normalize the signal so that the room acoustics, microphone quality, noise does not effect our algorithm
	// Let's consider the stanard maximum absolute intensity value to be 10,000. And then map all the values based on this
	// Find out the maximum absolute intensity for our data, find the normalization factor, then multiply that with each sample
	long double max_intensity = 0, standard_max_intensity = 10000.00;
	for(unsigned int i = 0; i < intensity.size(); i ++) if(abs(intensity[i]) > max_intensity) max_intensity = abs(intensity[i]);
	long double normalization_factor = standard_max_intensity / max_intensity;
	for(unsigned int i = 0; i < intensity.size(); i ++) intensity[i] *= normalization_factor;

	/*
	==========================================================
	Calculating the ZCR and STE values (with frame size : 320)
	==========================================================
	*/

	// "ste" : It stores the Short Term Energy (STE) values, for each frame of size 320
	// "zcr" : It stores the Zero Crossing Rate (ZCR) values, for each frame of size 320
	// We are calculating STE values like, "mean of squared intensity values" in each disjoint window of size 320
	// We are calculating ZCR values like, "No of times the signal crossed/touches the x axis" in each disjoint window of size 320
	vector<long double> ste;
	vector<unsigned int> zcr;
	unsigned int frameSize = 320, curFrame = 0, cur_zcr = 0;
	long double cur_ste = 0.0;
	for(unsigned int i = 0; i < intensity.size(); i ++){
		curFrame ++;
		cur_ste += (intensity[i] * intensity[i]);
		if(i > 0 && (intensity[i-1] * intensity[i] <= 0)) cur_zcr ++;
		if(curFrame == frameSize){
			cur_ste /= frameSize;
			ste.push_back(cur_ste);
			zcr.push_back(cur_zcr); 
			// To print the STE and ZCR values
			// printf("%u %.1Lf %u\n", ste.size(), ste[ste.size()-1], zcr[ste.size()-1]);
			cur_ste = 0.0;
			cur_zcr = 0;
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
	long double tot_ste = 0, tot_zcr = 0, avg_ste = 0, avg_zcr = 0;
	long long last_boundary = -1;
	for(unsigned int i = 0; i < ste.size(); i ++){
		tot_ste += ste[i]; 
		tot_zcr += zcr[i];
		avg_ste = tot_ste / (i-last_boundary+1);
		avg_zcr = tot_zcr / (i-last_boundary+1);
		if(i > 1){
			if(ste[i] >= avg_ste * 10 && ste[i] > 1000.0 && zcr[i] < 50 && avg_zcr > 30){
				start_boundary.push_back(i);
				last_boundary = i;
				tot_ste = ste[i];
				tot_zcr = zcr[i];
			}
			else if(ste[i] < avg_ste / 8000 && abs(zcr[i] - avg_zcr) <= 50){
				end_boundary.push_back(i-1);
				last_boundary = i;
				tot_ste = ste[i];
				tot_zcr = zcr[i];
			}
			else if(ste[i] < avg_ste / 8000 && zcr[i] >= 40 && avg_zcr < 40){
				end_boundary.push_back(i-1);
				last_boundary = i;
				tot_ste = ste[i];
				tot_zcr = zcr[i];
			}
		}
	}
	// To print the sound boundaries :-
	// for(unsigned int i = 0; i < start_boundary.size(); i ++) cout << start_boundary[i] << " "; cout << endl;
	// for(unsigned int i = 0; i < end_boundary.size(); i ++) cout << end_boundary[i] << " "; cout << endl;

	/*
	======================================
	Distinguishing the word (if YES or NO)
	======================================
	*/

	// Now as we have the boundary described for ourselves, we will distinguish the uttered word
	// I will distinguish based on the ZCR values
	// Due to the presence of "s" Fricative in the word "YES", the ZCR values at the ending half of word "YES" are very high, resulting in a high average ZCR for the last part
	// As the word "NO" consists of a nasal consonant "n" and vowel "o", both of them have very low ZCR values, resulting in a low average ZCR for the word at the last part
	// For my voice data, the "NO" have average ZCR strictly less than "50", where as for "YES" it is greater than "50", may even go more than "100"
	cout << "Here is the result for our implementation of YES/NO detection :-\n\n";
	for(unsigned int i = 0; i < min(start_boundary.size(), end_boundary.size()); i ++){
		tot_zcr = 0;
		unsigned int range = (unsigned int)(end_boundary[i] - start_boundary[i] + 1);
		unsigned int considered_range = range / 2;

		for(unsigned int j = (unsigned int)start_boundary[i] + considered_range; j <= (unsigned int)end_boundary[i]; j ++){
			tot_zcr += zcr[j];
		}
		avg_zcr = tot_zcr / (end_boundary[i] - start_boundary[i] - considered_range + 1);
		if(avg_zcr < 50) cout << "NO  (starting from frame : " << start_boundary[i] << ", ending at frame : " << end_boundary[i] << ", having avg. ZCR (at the ending half) = " << avg_zcr << " [ < 50])\n";
		else if(avg_zcr >= 50) cout << "YES (starting from frame : " << start_boundary[i] << ", ending at frame : " << end_boundary[i] << ", having avg. ZCR (at the ending half) = " << avg_zcr << " [ >= 50])\n";
	} cout << endl;

	return 0;
}

