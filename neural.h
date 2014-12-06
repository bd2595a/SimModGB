/*Defines a Perceptron class and constants
Michael Black, 10/2014
If you want to use the stuff in neural.cpp, put #include "neural.h" in your cpp file
*/

//the learning rate alpha
//in general, a small alpha will learn more slowly and may get stuck in local minima
//a big alpha, however, may jump over the solution rather than converge on it
const float alpha = 0.05;
//the magnitude of the initial weight values range uniformly from -noisemax/2 to noisemax/2
const float noisemax = 0.4;

//define a Perceptron object
//Perceptron* neuralnet = new Perceptron(3); makes a neural net with 3 inputs and 3 hidden nodes
class Perceptron
{
public:
	//neuralnet->getPrediction(inputs) takes an array of integer inputs and produces a binary guess
	int getPrediction(int*);
	//neuralnet->getRawPrediction(inputs) does the same, but produces a floating point guess
	float getRawPrediction(int*);
	//neuralnet->train(inputs, desiredoutput) teaches the neural net to give the desiredoutput when those inputs are seen
	bool train(int*, int);//true for if it's trained, false if it hasn't
	//constructor
	Perceptron(int);
	//array of size+1 output layer weights
	float* outputweight;
	//2D array: an array of size+1 hidden layer weights for each of size hidden nodes
	float** hiddenweight;
	//the number of inputs and hidden nodes
};
