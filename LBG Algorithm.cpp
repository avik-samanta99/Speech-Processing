#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <algorithm>
#include <time.h>
using namespace std;
#define dimension 12 // P = 12, means we are using 12-dimensional codebook vector
#define delta 0.00001 // This is the threshold value for the change in the distortion
#define epsilon 0.03 // This is the parameter based on which we splitt the codebook vectors

int k, K; // k : current codebook size, K : Expected codebook size 
vector<vector<long double>> universe; // Universe of the Training Dataset 

/*
=============================================================================================
Function to split the codebook vectors, to double the size of the codebook
-> For each vector, for each element we will use the parameter epsilon
-> And generate two values, which will be the elements of respective index of two new vectors 
=============================================================================================
*/
void splitCodebookVectors(vector<vector<long double>> &refCodebook, vector<vector<long double>> &codebook){
	for(int i = 0; i < (int)refCodebook.size(); i ++){
		for(int j = 0; j < dimension; j ++){
			codebook[2 * i][j] = refCodebook[i][j] * (1 - epsilon);
			codebook[2 * i + 1][j] = refCodebook[i][j] * (1 + epsilon);
		}
	}
}

/*
===================================================================================
Function to calculate the Tokhura's Distance, given two Vectors [p(12)-dimentional]
===================================================================================
*/
void tokhuraDistance(vector<long double> &reference, vector<long double> &test, long double &distance){
	// These are Tokhura's weights (already provided)
	long double tokhuraWeights[] = {1.0, 3.0, 7.0, 13.0, 19.0, 22.0, 25.0, 33.0, 42.0, 50.0, 56.0, 61.0};
	distance = 0.0;
	for(int i = 0; i < dimension; i ++)
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
void KClusters(vector<vector<vector<long double>>> &clusters, vector<vector<long double>> &codebook){
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
==============================================================================
This function takes care of the Empty Cell problem, if there is any
-> It finds the clusters which are empty (has no training vector mapped to it)
-> Then from the clusters which have most number of vectors mapped to it,
-> Half of vectors from those clusters are transfered to the empty cells
==============================================================================
*/
void emptyCell(vector<vector<vector<long double>>> &cluster){
	vector<int> emptyCells; // Clusters which have no vectors assigned to them
	vector<pair<int, int>> frequency; // frequencies of each cluster
	for(int i = 0; i < k; i ++){
		if((int)cluster[i].size() == 0)
			emptyCells.push_back(i);
		frequency.push_back(make_pair((int)cluster[i].size(), i));
	}

	// sort the clusters based on their frequency
	sort(frequency.begin(), frequency.end());

	for(int i = 0; i < (int)emptyCells.size(); i ++){
		int destinationCluster = emptyCells[i]; // pick one empty cell, that is our destination cluster
		int targetCluster = frequency[k - 1 - i].second; // the cluster with maximum frequency is chosen for the target cluster
		int transferVectors = frequency[k - 1 - i].first / 2; // number of vectors to be transfered
		int targetClusterSize = (int)cluster[targetCluster].size(); // size or frequency of the target cluster
		for(int j = 0; j < transferVectors; j ++){ // pick one vector from target cluster, transfer it to the empty cell
			cluster[destinationCluster].push_back(cluster[targetCluster][targetClusterSize - 1 - j]);
			cluster[targetCluster].pop_back();
		}
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
	for(int d = 0; d < dimension; d ++){
		totValue = 0.0;
		for(int j = 0; j < (int)cluster.size(); j ++){
			totValue += cluster[j][d];
		}
		codebookVector[d] = totValue / (long double) cluster.size();
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
		for(int j = 0; j < dimension; j ++)
			printf("%llf ", codebook[i][j]);
	    cout << endl;
	} cout << endl;
}

/*
=============================================================
This is the driver funcion to implement the K-Means Algorithm
=============================================================
*/
void Split_KMeans(vector<vector<long double>> &refCodebook){

	// codebook : This is vector representing the current codebook 
	vector<vector<long double>> codebook(k, vector<long double> (dimension));
	// cluster : vector reprsenting the clusters of the training vectors
	vector<vector<vector<long double>>> cluster(k);
	// Variables to keep track of the distortions (current and previous)
	long double avgDistortion, prevAvgDistortion;

	// Split the codebook to double the size of it
	splitCodebookVectors(refCodebook, codebook);
	cout << "Our codebook after splitting the previous codebook :-\n"; 
	displayCodebook(codebook);

	// Cluster the training vectors based on the new codebook generated after splitting
	KClusters(cluster, codebook);

	// Takes care of empty cell problem
	emptyCell(cluster);

	// compute the average distortion for the current codebook 
	computeDistortion(cluster, codebook, avgDistortion);

	// apply the K-means algorithm to find the (approximately) best codebook of size current k
	int iteration = 0;
	cout << "Let's start the K-Means Algorithm :-\n";
    cout << "K-Means Iteration " << iteration << " : Average Distortion -> " << avgDistortion << endl;
	do{
		iteration ++;
		prevAvgDistortion = avgDistortion;
		// update the codebook
		updateCodebook(cluster, codebook);
		for(int i = 0; i < k; i ++) cluster[i].clear();

		// cluster the training vectors
		KClusters(cluster, codebook);

		// compute the average distortion again, and see if that reaches the threshold or not
		computeDistortion(cluster, codebook, avgDistortion);
		cout << "K-Means Iteration " << iteration << " : Average Distortion -> " << avgDistortion << ", Change(delta) -> ";
		printf("%.7llf\n", abs(avgDistortion - prevAvgDistortion));

	} while(abs(avgDistortion - prevAvgDistortion) > delta);

	cout << "At the end we get the final Average Distortion : " << avgDistortion << endl;
	
	// Update the previous codebook with new codebook (with double the size)
	refCodebook = codebook;
}

int main(){
	// Input file stream
	ifstream fin;
	fin.open("universe.csv");
    string line, word;
    char temp;

	// read the information line by line
    while (getline(fin, line)){
		vector<long double> row(12);
		// convert into a string stream 
        stringstream str(line);
		// Then get the values, separated by comma's
		for(int i = 0; i < dimension; i ++)
			str >> row[i] >> temp;
		// store the data into the vector representing the universe / training dataset
		universe.push_back(row);
    }

	// let the user decide the size of the codebook
	cout << "Enter the size of the Codebook, K = ";
	cin >> K;

	// the codebook (initially with 1 refrence vector)
	vector<vector<long double>> codebook(1, vector<long double> (dimension));

	// initially the codebook contains one vector, which is the centroid of the universe 
	centroid(universe, codebook[0]);
	cout << "\nThe centroid of the Universe (The initial Codebook) :-\n";
	k = codebook.size();
	displayCodebook(codebook);
	int iterationCount = 1;

	while((int)codebook.size() < K){ // Repeat until we have a codebook of size K
		cout << "                                 LBG Iteration Number : " << iterationCount ++ << endl << endl;
		k = codebook.size() * 2;
	    // double the size (split) of the codebook, then apply K-Means
		Split_KMeans(codebook);
		cout << "\nThe codebook after applying the K-Means Algorithm :-\n";
		displayCodebook(codebook); cout << endl;
	}

	return 0;
}