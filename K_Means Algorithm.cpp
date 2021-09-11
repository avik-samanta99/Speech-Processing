#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <time.h>
using namespace std;
#define dimention 12 // P = 12, means we are using 12-dimensional codebook vector
#define delta 0.00001 // This is the threshold value for the change in the distortion

int k; // k : size of the codebook

/*
=================================================================================
Function to initialize the codebook (of size k)
-> Select k random vectors from the universe, & construct the codebook with those
=================================================================================
*/
void initialization(vector<vector<long double>> &codebook, vector<vector<long double>> &universe){
	int universeSize = universe.size();
	for(int i = 0; i < k; i ++){
		int randomIndex = rand() % universeSize;
		codebook[i] = universe[randomIndex];
	}
}

/*
===================================================================================
Function to calculate the Tokhura's Distance, given two Vectors [p(12)-dimentional]
===================================================================================
*/
void tokhuraDistance(vector<long double> &reference, vector<long double> &test, long double &distance){
	long double tokhuraWeights[] = {1.0, 3.0, 7.0, 13.0, 19.0, 22.0, 25.0, 33.0, 42.0, 50.0, 56.0, 61.0};
	distance = 0.0;
	for(int i = 0; i < dimention; i ++)
		distance += tokhuraWeights[i] * (reference[i] - test[i]) * (reference[i] - test[i]);
}

/*
================================================================================================
Function to create K-Clusters from the training dataset, based on the reference codebook vectors
-> What we are doing here is that, we are taking each training vector from universe one by one
-> Then computing the Tokhura's Distance from each of the reference codebook vectors
-> Then we cluster that vector with the least (Tokhura's) distant reference codebook vector
================================================================================================
*/
void KClusters(vector<vector<vector<long double>>> &clusters, vector<vector<long double>> &universe, vector<vector<long double>> &codebook){
	long double distance, minDistance;
	int clusterNo;
	for(int i = 0; i < (int)universe.size(); i ++){
		minDistance = 1e10;
		for(int j = 0; j < k; j ++){
			tokhuraDistance(codebook[j], universe[i], distance);
			if(distance < minDistance){
				minDistance = distance;
				clusterNo = j;
			}
		}
		clusters[clusterNo].push_back(universe[i]);
	}
}

/*
=================================================================================================================
This function computes the average distortion of the training vectors w.r.t the current codebook vectors
-> for each cluster, we are computing the distance of that vector from the reference vector for that cluster
-> We add all of those distances together, then divide by the size of the universe, to get the average distortion
=================================================================================================================
*/
void computeDistortion(vector<vector<vector<long double>>> &cluster, vector<vector<long double>> &codebook, long double &avgDistortion){
	long double totalDistortion = 0.0, curDistortion;
	int totVectors = 0;
	for(int i = 0; i < k; i ++){
		for(int j = 0; j < (int)cluster[i].size(); j ++){
			tokhuraDistance(codebook[i], cluster[i][j], curDistortion);
			totalDistortion += curDistortion;
		}
		totVectors += cluster[i].size();
	}
	avgDistortion = totalDistortion / (long double)totVectors;
}

/*
================================================================================
This function computes the centroid of a given cluster
-> Given a cluster (a set of vectors), finds out the centroid of those vectors
-> Actually it computes the mean [for each i = 1 -> P(12)] of the set of vectors
================================================================================
*/
void centroid(vector<vector<long double>> &cluster, vector<long double> &codebookVector){
	long double totValue;
	for(int d = 0; d < dimention; d ++){
		totValue = 0.0;
		for(int j = 0; j < (int)cluster.size(); j ++){
			totValue += cluster[j][d];
		}
		codebookVector[d] = totValue / (long double)cluster.size();
	}
}

/*
====================================================================================================================
This function is used to update the codebook reference vectors, based on the cluster we just created
-> For the cluster, respect to a reference vector, we find the centroid, and replace the reference vector with this 
====================================================================================================================
*/
void updateCodebook(vector<vector<vector<long double>>> &cluster, vector<vector<long double>> &codebook){
	for(int i = 0; i < k; i ++){
		centroid(cluster[i], codebook[i]);
	}
}

/*
===========================================================
This is just to display the current content of the codebook
===========================================================
*/
void displayCodebook(vector<vector<long double>> &codebook){
	for(int i = 0; i < k; i ++){
		cout << i << " ->  ";
		for(int j = 0; j < dimention; j ++)
			printf("%0.6llf ", codebook[i][j]);
	    cout << endl;
	} cout << endl;
}

int main(){
	// srand() : To choose random number, but each time new 
	srand((int)time(0));

    // let the user decide the size of the codebook
	cout << "Enter the size of the Codebook, K = ";
	cin >> k;

	// Input file stream
	ifstream fin;
	fin.open("universe.csv");
	vector<vector<long double>> universe; 
    string line, word;
    char temp;

    // read the information line by line
    while (getline(fin, line)){
		vector<long double> row(12);
		// convert into a string stream
        stringstream str(line);
		// Then get the values, separated by comma's
		for(int i = 0; i < dimention; i ++)
			str >> row[i] >> temp;
		// store the data into the vector representing the universe / training dataset
		universe.push_back(row);
    } 

    // The vector representing the codebook
	vector<vector<long double>> codebook(k, vector<long double> (12));
	// the vector representing all the clusters
	vector<vector<vector<long double>>> cluster(k);
	// Variables to keep track of the distortions (current and previous)
	long double avgDistortion, prevAvgDistortion;

    // Initialise our codebook randomly
	initialization(codebook, universe);
	cout << "Initial codebook (Randomly chosen from the universe) looks like :-\n"; 
	displayCodebook(codebook);

	// Cluster the training vectors based on the new codebook generated after splitting
	KClusters(cluster, universe, codebook);

	// compute the average distortion for the current codebook 
	computeDistortion(cluster, codebook, avgDistortion);
	int iteration = 0;
	cout << "Let's start the K-Means Algorithm :-\n";
    cout << "Iteration " << iteration << " : Average Distortion -> " << avgDistortion << endl;

	// apply the K-means algorithm to find the (approximately) best codebook of size current k
	do{
		iteration ++;
		prevAvgDistortion = avgDistortion;
		// update the codebook
		updateCodebook(cluster, codebook);
		for(int i = 0; i < k; i ++) cluster[i].clear();

		// cluster the training vectors
		KClusters(cluster, universe, codebook);

	    // compute the average distortion again, and see if that reaches the threshold or not
		computeDistortion(cluster, codebook, avgDistortion);
		cout << "Iteration " << iteration << " : Average Distortion -> " << avgDistortion << ", change(delta) -> ";
		printf("%0.7llf\n", abs(avgDistortion - prevAvgDistortion));
	} while(abs(avgDistortion - prevAvgDistortion) > delta);
	
	cout << "At the end we get the final Average Distortion : " << avgDistortion << endl;
	cout << "\nThe final codebook we get after the end of the K-Means algorithm :-\n";
	displayCodebook(codebook);

	return 0;
}