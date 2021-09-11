/*
=============================
Name : Avik Samanta
Roll no. - 204101016
Asgn. 3 - Vowel recognition
Submission Date : 04/11/2020
=============================
*/

#include "stdio.h"
#include "stdlib.h"
#include "vector"
#include "iostream"
#include "fstream"
#include "string"
using namespace std;
#define pi 3.14286 // value of pi (22/7), defined beforehand, so that can be used later
#define frameSize 320 // Frame of 20ms (with a rate of 16k samples per second) = 320 samples
#define throwFrame 20 // throw initial 20 frames (to discard the initial disturbances)
#define DCShiftFrame 10 // Initial 10 frames of silences are used for computation of DC shift amount
#define steadyFrames 5 // We will use 5 frames from the steady state of the vowel utterance
#define standardMaxIntensity 10000.0 // Standard Maximum (absolute) Intensity is set to 10k here
#define LPCOrder 12 // LPC model order = no. of coefficients (Ai, Ci) = P = 12

/*
================================================================================
This function reads the intensity values (sample values) stored in a file (.txt)
================================================================================
*/
void ReadIntensity(ifstream &inPtr, vector<long double> &intensity){
    // First remove initial  lines, that may contain the header of the file
	string str;
	for(unsigned int j = 0; j < 5; j ++) 
		getline(inPtr, str);

	// Then read the sample values one by one from the file
	long double curIntensity;
	inPtr >> curIntensity;
	while(inPtr){
		intensity.push_back((long double)curIntensity);
		inPtr >> curIntensity;
	}
}

/*
===========================================================================================================
This function throw the initial disturbances from the sound signal
-> Even though, the already recorded files are already trimmed we are not going to use this for those files
-> But the live recording may have that initial disturbance part, we will use this function to remove them
===========================================================================================================
*/
void ThrowInitialDisturbance(vector<long double> &speechIntensity){
	speechIntensity.erase(speechIntensity.begin(), speechIntensity.begin() + (frameSize * throwFrame)); 
}

/*
=============================================
This function applies DC Shift to the signal
=============================================
*/
void DCShift(vector<long double> &speechIntensity){
	// Find the Shift amount from the initial 10 frames of silence
	long double totalIntensity = 0.0, avgIntensity;
	for(unsigned int i = 0; i < (DCShiftFrame * frameSize); i ++) 
		totalIntensity += speechIntensity[i];

	// Apply that shift to the whole signal
	avgIntensity = totalIntensity / (DCShiftFrame * frameSize);
	for(unsigned int i = 0; i < speechIntensity.size(); i ++) 
		speechIntensity[i] -= avgIntensity;
}

/*
===========================================================
This function is for the Normalization of the speech signal
===========================================================
*/
void Normalization(vector<long double> &speechIntensity){
	// Find out the maximum absolute intensity
	long double maxIntensity = 0;
	for(unsigned int i = 0; i < speechIntensity.size(); i ++)
		if(abs(speechIntensity[i]) > maxIntensity)
			maxIntensity = speechIntensity[i];

	// Normalize the whole signal by multiplying the normalization factor
	long double normalizationFactor = standardMaxIntensity / maxIntensity;
	for(unsigned int i = 0; i < speechIntensity.size(); i ++)
		speechIntensity[i] *= normalizationFactor;
}

/*
================================================================
This function calculates the Short Term Energy of a single fram
-> We will use this for the steady state computation
================================================================
*/
long double STE(vector<long double> &speechIntensity, unsigned int frame){
	long double ste = 0.0;
	for(unsigned int i = (frame * frameSize); i < ((frame + 1) * frameSize); i ++)
		ste += (speechIntensity[i] * speechIntensity[i]);
	ste /= frameSize;
	return ste;
}

/*
===============================================================================================
This function gives us the 5 frames from the steady state of the signal
-> The section with the highest Short Term energy level is considered the steady state
-> We will take 2 frames before the max energy frame, and two frames after the max energy frame
===============================================================================================
*/
void SteadyState(vector<long double> &speechIntensity){
	// Find the number of complete frames available
	unsigned int frames = speechIntensity.size() / frameSize;

	// remove the incomplete last frame
	speechIntensity.erase(speechIntensity.begin() + (frames * frameSize), speechIntensity.end());

	// Find out the frame with maximum STE (Energy)
	unsigned int maxEnergyFrame;
	long double maxEnergy = 0;
	for(unsigned int i = 0; i < frames; i ++){
		long double ste = STE(speechIntensity, i);
		if(ste > maxEnergy){
			maxEnergy = ste;
			maxEnergyFrame = i;
		}
	}

	// Keep two frames before and after the max energy frame, and remove the remaining part
	speechIntensity.erase(speechIntensity.begin(), speechIntensity.begin() + ((maxEnergyFrame - 2) * frameSize));
	speechIntensity.erase(speechIntensity.begin() + (5 * frameSize), speechIntensity.end());
}

/*
==================================================
Apply the Hamming Window on the each of the Frames
==================================================
*/
void HammingWindow(vector<long double> &speechIntensity, unsigned int frameNo){
	unsigned int start = (frameNo * frameSize);
	for(unsigned int j = start; j < (start + frameSize); j ++){
		long double weight = 0.54 - 0.46 * cos(2 * pi * (long double)(j - start) / (long double)(frameSize -1));
		speechIntensity[j] *= weight;
	}
}

/*
=========================================================
Find out the Auto-correlation values (Ri's) for the frame
=========================================================
*/
void AutoCorrelation(vector<long double> &speechIntensity, vector<long double> &CorrelationVector, unsigned int frameNo){
	unsigned int start = (frameNo * frameSize);
	for(unsigned int j = 0; j <= LPCOrder; j ++){
		CorrelationVector.push_back(0.0);
		for(unsigned int k = 0; k < (frameSize - j); k ++)
			CorrelationVector[j] += speechIntensity[start + k] * speechIntensity[start + k + j];
	}
}

/*
====================================================================
Find out the Prediction Coefficients (Ai's) using Durbin's Algorithm
====================================================================
*/
void LevinsonDurbin(vector<long double> &CorrelationVector, vector<long double> &predictionCoefficients){
	long double alpha[LPCOrder + 1][LPCOrder + 1];
	vector<long double> residualError(LPCOrder + 1);
	residualError[0] = CorrelationVector[0];
	for(unsigned int j = 1; j <= LPCOrder; j ++){
		long double subtract = 0.0;
		for(unsigned int k = 1; k < j; k ++)
			subtract += alpha[j-1][k] * CorrelationVector[j-k];
		alpha[j][j] = (CorrelationVector[j] - subtract) / residualError[j-1];
		for(unsigned int k = 1; k < j; k ++) 
			alpha[j][k] = alpha[j-1][k] - alpha[j][j] * alpha[j-1][j-k];
		residualError[j] = (1 - alpha[j][j] * alpha[j][j]) * residualError[j-1];
	}
	predictionCoefficients.push_back(0);
	for(unsigned int j = 1; j <= LPCOrder; j ++) 
		predictionCoefficients.push_back(alpha[LPCOrder][j]);
}

/*
==================================================================================
Find out the Cepstrul Coefficients (Ci's), using the Prediction Coefficients(Ai's)
==================================================================================
*/
void CepstrulCoefficients(vector<long double> &predictionCoefficients, vector<long double> &cepstrulCoefficients, long double ste){
	cepstrulCoefficients.push_back(log(ste * ste));
	for(unsigned int i = 1; i <= LPCOrder; i ++){
		cepstrulCoefficients.push_back(predictionCoefficients[i]);
		for(unsigned int j = 1; j < i; j ++){
			cepstrulCoefficients[i] += ((long double)j * cepstrulCoefficients[j] * predictionCoefficients[i-j] / (long double)i);
		}
	}
}

/*
=====================================================
Apply Raised Sine Window on the Cepstrul Coefficients
=====================================================
*/
void RaisedSineWindow(vector<long double> &cepstrulCoefficients){
	for(int i = 0; i < LPCOrder; i ++){
		long double w = 1 + (long double)LPCOrder * sin(pi * (long double)(i+1) / (long double)LPCOrder) / 2.0;
		cepstrulCoefficients[i] *= w;
	}
}

/*
==================================================================================================================
This function computes the Tokhura's Distance, given two Vowel Representatives (5 frames of Cepstrul Coefficients)
==================================================================================================================
*/
void TokhuraDistance(vector<vector<long double>> &refVowel, vector<vector<long double>> &curVowel, long double &distance){
	long double tokhuraWeights[12] = {1.0, 3.0, 7.0, 13.0, 19.0, 22.0, 25.0, 33.0, 42.0, 50.0, 56.0, 61.0};
	distance = 0.0;
	for(unsigned int i = 0; i < steadyFrames; i ++){
		for(unsigned int j = 0; j < LPCOrder; j ++){
			distance += tokhuraWeights[j] * (refVowel[i][j] - curVowel[i][j]) * (refVowel[i][j] - curVowel[i][j]);
		}
	}
}

/*
=======================================================================================================================================
Given the sample values, this function extract the vowel (sound) representative, which is :-
-> Representative : Cepstrul Coefficients (Ci's) of 5 Steady Frames from the signal, 2D vector of size 5 * 12 (5 frames * 12 Ci's each)
-> Steps :-
1. Apply DC shift
2. Apply Normalization
3. Apply Hamming window (on each of the frames)
4. Find out the steady states
5. Find auto-correlation values (For each steady state)
6. Find the Ai's using Durbin Algorithm (For each steady state)
7. Find the Ci's (For each steady state)
8. Apply raised sine window (For each steady state)
=======================================================================================================================================
*/
void RepresentativeCepstrulCoefficients(vector<long double> &intensity, vector<vector<long double>> &correlationR, vector<vector<long double>> &coefficientsA, vector<vector<long double>> &coefficientsC){
	DCShift(intensity);
	Normalization(intensity);
	for(unsigned int j = 0; j < (unsigned int)(intensity.size() / frameSize); j++)
		HammingWindow(intensity, j);
	SteadyState(intensity);
	for(unsigned int j = 0; j < steadyFrames; j ++){
		AutoCorrelation(intensity, correlationR[j], j);
		LevinsonDurbin(correlationR[j], coefficientsA[j]);
		CepstrulCoefficients(coefficientsA[j], coefficientsC[j], correlationR[j][0]);
		coefficientsC[j].erase(coefficientsC[j].begin());
		RaisedSineWindow(coefficientsC[j]);
	}
}

/*
=============================================================================
This is to display the Reference cepstrul Coefficients for each of the vowels
=============================================================================
*/
void ShowReferenceCepstrulCoefficients(vector<vector<vector<long double>>> &refCepstrulCoefficients){
	char vowel[5] = {'a', 'e', 'i', 'o', 'u'};
    cout << "Results we get after training the data we had :-\n\n";
	for(unsigned int v = 0; v < 5; v ++){
	    cout << "Reference Cepstrul Coefficients for Vowel : \'" << vowel[v] << "\'" << endl;
		for(unsigned int j = 0; j < steadyFrames; j ++){
			for(unsigned int k = 0; k < LPCOrder; k ++){
				printf("%Lf ",refCepstrulCoefficients[v][j][k]);
			}
			cout << endl;
		}
		cout << endl;
	}
}

/*
=======================
Driver (main) function
=======================
*/
int main()
{
	string str;
	char vowel[5] = {'a', 'e', 'i', 'o', 'u'};

	// Files (directory) to train our system
	char trainDirectory[30] = "recordings/204101016_x_0y.txt";
	char refFile[20] = "204101016_ref_x.txt";

	// Vector to store the Reference Cepstrul Coefficients of each of the vowels
	vector<vector<vector<long double>>> refCepstrulCoefficients(5);

	for(unsigned int v = 0; v < 5; v ++){
        // Changing the filename, to train different vowels
		trainDirectory[21] = vowel[v];

		// vector to store the cepstrul coefficients for 10 files for each of the vowel
		vector<vector<vector<long double>>> testCepstrulCoefficients(10);

		for(unsigned int i = 0; i < 10; i ++){
			// modifying the filename, to train different utterances
			trainDirectory[24] = '0' + i;

			// open the file with the samples of the vowel
			ifstream inPtr;
			inPtr.open(trainDirectory);

			// vectors to store intensities, Ri, Ai, Ci's
			vector<long double> intensity;
			vector<vector<long double>> correlationR(steadyFrames);
			vector<vector<long double>> coefficientsA(steadyFrames);
			vector<vector<long double>> coefficientsC(steadyFrames);
			
			// Do the calculations
			ReadIntensity(inPtr, intensity);
			RepresentativeCepstrulCoefficients(intensity, correlationR, coefficientsA, coefficientsC);

			// Store the derived representatives
			testCepstrulCoefficients[i] = coefficientsC;
		}

		// Compute the average of the 10 train files for one vowel
		vector<vector<long double>> curRefCepstrulCoefficients(steadyFrames);
		for(unsigned int j = 0; j < steadyFrames; j ++){
			for(unsigned int k = 0; k < LPCOrder; k ++){
				long double temp = 0.0;
				for(unsigned int i = 0; i < 10; i ++)
					temp += testCepstrulCoefficients[i][j][k];
				curRefCepstrulCoefficients[j].push_back(temp / 10.0);
			}
		}

		// store the reference for the vowel
		refCepstrulCoefficients[v] = curRefCepstrulCoefficients;

		refFile[14] = vowel[v];
		ofstream outPtr;
		outPtr.open(refFile);
		for(unsigned int j = 0; j < steadyFrames; j ++){
			for(unsigned int k = 0; k < LPCOrder; k ++){
				outPtr << refCepstrulCoefficients[v][j][k] << " ";
			}
			outPtr << "\n";
		}
	}

	// display the references for each of the vowel
	ShowReferenceCepstrulCoefficients(refCepstrulCoefficients);

	// Now is the testing time
	cout << "Now let's test our system...\n\n";
	while(true){
		int choice;
		cout << "1. Test with already recorded voice data,\n";
		cout << "2. Test with a live recording,\n";
		cout << "3. Stop testing and Terminate the program.\n";
		cout << "-> Enter your choice : ";
		cin >> choice;
		
		switch(choice){
		    case 1:{ // this tests all the recorded files, and predicts the vowels, and computes the accuracy at the end
				int correctCount = 0, totalCount = 0;
				char testDirectory[30] = "recordings/204101016_x_1y.txt";
				for(unsigned int i = 0; i < 5; i ++){
					testDirectory[21] = vowel[i];
					for(unsigned int l = 0; l < 10; l ++){
						testDirectory[24] = '0' + l;
						ifstream inPtr(testDirectory);

						vector<long double> intensity;
						vector<vector<long double>> correlationR(steadyFrames);
						vector<vector<long double>> coefficientsA(steadyFrames);
						vector<vector<long double>> coefficientsC(steadyFrames);

						ReadIntensity(inPtr, intensity);
						RepresentativeCepstrulCoefficients(intensity, correlationR, coefficientsA, coefficientsC);

						long double minDistance = (long double)1e15, curVowelDistance;
						char targetVowel;
						for(unsigned int j = 0; j < 5; j ++){
							TokhuraDistance(refCepstrulCoefficients[j], coefficientsC, curVowelDistance);
							if(curVowelDistance < minDistance){
								minDistance = curVowelDistance;
								targetVowel = vowel[j];
							}
						}
						cout << "The actual vowel is : " << vowel[i] << ", the predicted vowel is : " << targetVowel << ". Prediction : ";
						if(vowel[i] == targetVowel)
							cout << "Correct\n", correctCount ++;
						else cout << "Wrong\n";
						totalCount ++;
					}
				}
				cout << "Prediction Accuracy (only for the recorded data) : " << (long double)(correctCount / (long double)totalCount) * 100 << " %" << endl;
				cout << endl;
				break;
		    }
			case 2:{ // This tests the live recording, for real time testing experience
				system("Recording_Module.exe 3 input_file.wav input_file.txt");
				string testDirectory = "input_file.txt";
				ifstream inPtr(testDirectory);

				vector<long double> intensity;
				vector<vector<long double>> correlationR(steadyFrames);
				vector<vector<long double>> coefficientsA(steadyFrames);
				vector<vector<long double>> coefficientsC(steadyFrames);

				ReadIntensity(inPtr, intensity);
				ThrowInitialDisturbance(intensity);
				RepresentativeCepstrulCoefficients(intensity, correlationR, coefficientsA, coefficientsC);

				long double minDistance = (long double)1e15, curVowelDistance;
				char targetVowel;
				for(unsigned int j = 0; j < 5; j ++){
					TokhuraDistance(refCepstrulCoefficients[j], coefficientsC, curVowelDistance);
					if(curVowelDistance < minDistance){
						minDistance = curVowelDistance;
						targetVowel = vowel[j];
					}
				}
				cout << "The vowel our program has predicted is : \'" << targetVowel << "\'" << endl << endl;
				break;
		    }
			case 3:{ // This is where the program should halt
				cout << "Thank you! That was a lot of fun ... \n\n";
				exit(0);
		    }
			default:{
				cout << "Invalid entry! Try again...\n";
			}
		}
	}
	return 0;
}