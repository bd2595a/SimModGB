/* Implements a multilayer neural network
Michael Black, 10/2014*/

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <math.h>

//class definition and global constants are in here
#include "neural.h"

using namespace std;

//implements the threshold function 1/(1+e^-x)
//this is mathematically close to the >=0 threshold we use in the single layer perceptron, but is differentiable
float sigmoid(float x)
{
	return 1/(1+pow(2.71828,-x));
}

//constructor.  Called with the number of inputs:  new Perceptron(3) makes a three input perceptron.
Perceptron::Perceptron(int nodes)
{
	//save the input size for later
	size=nodes;
	//make an array of output weights, +1 for the bias
	outputweight=new float[nodes+1];
	//set each weight to a random real number between -noisemax/2 .. noisemax/2
	for(int i=0; i<nodes+1; i++)
		outputweight[i]=(float)(((rand()%1000) / 1000.0)*noisemax - noisemax/2);
	//make a 2D array of hidden weights
	hiddenweight = new float*[nodes];
	for(int i=0; i<nodes; i++)
	{
		//each hidden node will get a set of hidden weights, one for each input, plus a bias
		hiddenweight[i]=new float[nodes+1];
		//make them random too
		for(int j=0; j<nodes+1; j++)
			hiddenweight[i][j]=(float)(((rand()%1000) / 1000.0)*noisemax - noisemax/2);
	}

}

//takes an array of binary (1/0) inputs, feeds them to the perceptron, returns a binary guess: 1/0
int Perceptron::getPrediction(int* inputs)
{
	//store the output of the hidden nodes here
	float* hidden = new float[size];

	//for each hidden node
	for (int hn=0; hn<size; hn++)
	{
		//calculate a dot product: input0 * weight0 + input1 * weight1 ...
		float sum = 0;
		for (int i=0; i<size; i++)
			//(inputs[i]==0?-1:1) makes the input value being multiplied -1 or 1 instead of 0 or 1
			sum += (inputs[i]==0?-1:1) * hiddenweight[hn][i];
		//add on the bias weight times a fixed input of 1
		sum+=hiddenweight[hn][size];
		//pass the sum through the threshold function
		hidden[hn] = sigmoid(sum);
	}

	//now do it again for the output stage
	float sum=0;
	for(int i=0; i<size; i++)
	{
		sum+= hidden[i] * outputweight[i];
	}
	sum+=outputweight[size];
	//return the result of the output stage, rounded to the nearest 0 or 1.
	return sigmoid(sum)>=0.5? 1:0;
}

//this is identical to the above function, but it doesn't do the rounding
//this is useful when we want to compare two perceptron outputs and choose the more confident
//for example, a perceptron giving 0.9 is more "trustworthy" than a perceptron giving 0.6
float Perceptron::getRawPrediction(int* inputs)
{
	float* hidden= new float[size];

	for (int hn=0; hn<size; hn++)
	{
		float sum = 0;
		for (int i=0; i<size; i++)
			sum += (inputs[i]==0?-1:1) * hiddenweight[hn][i];
		sum+=hiddenweight[hn][size];
		hidden[hn] = sigmoid(sum);
	}

	float sum=0;
	for(int i=0; i<size; i++)
	{
		sum+= hidden[i] * outputweight[i];
	}
	sum+=outputweight[size];
	return sum;
}


//this trains the perceptron on an array of inputs (1/0) and a desired output (1/0)
//it returns TRUE if it predicts correctly and does not need further training
bool Perceptron::train(int* inputs, int want)
{
	//this is identical to getPrediction.  We need to get a prediction so that we can determine the error.
	float* hidden= new float[size];

	for (int hn=0; hn<size; hn++)
	{
		float sum = 0;
		for (int i=0; i<size; i++)
			sum += (inputs[i]==0?-1:1) * hiddenweight[hn][i];
		sum+=hiddenweight[hn][size];
		hidden[hn] = sigmoid(sum);
	}

	float sum=0;
	for(int i=0; i<size; i++)
	{
		sum+= hidden[i] * outputweight[i];
	}
	sum+=outputweight[size];
	float prediction =  sigmoid(sum);

	//now we have a prediction and we can train.

	//get an error.  the error is then multiplied by doutput/dhiddenoutput - essentially dthreshold() - to make its effect incremental
	//the derivative of the sigmoid threshold function is x*(1-x)
	float error = (want - prediction) * prediction * (1-prediction);
	float* hiddenerror = new float[size+1];

	//for each hidden node,
	for(int i=0; i<size; i++)
	{
		//calculate an error for the node.  multiply it by dhiddenoutput/dhiddeninput
		hiddenerror[i]=hidden[i]*(1-hidden[i])*outputweight[i]*error;
		//adjust the weight leading from that hidden node to the output
		//multiply it by alpha, a learning rate.
		outputweight[i]+= error * hidden[i] * alpha;
	}
	//don't forget the bias
	outputweight[size]+=alpha * error;
	//now go through each hidden node and train its inputs
	for(int hn=0; hn<size; hn++)
	{
		//for each input
		for(int i=0; i<size; i++)
		{
			//adjust the weight by that node's error.  the input must be treated as -1/1, not 0/1
			hiddenweight[hn][i]+= alpha * hiddenerror[hn] * (inputs[i]==0?-1:1);
		}
		//do it again for the bias
		hiddenweight[hn][size]+= alpha * hiddenerror[hn];
	}
	//return TRUE if the prediction was the desired output, FALSE otherwise
	return ((prediction>=0.5?1:0) == want);
}
