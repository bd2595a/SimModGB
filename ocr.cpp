/*A character recognizer that uses neural nets

TODO: YOUR NAME HERE, 10/2014

assignment and helper code by Michael Black, 10/2014

TODO:
YOUR CODE WILL GO IN FUNCTIONS test() AND train()
HERE STATE WHAT STEPS YOU ACCOMPLISHED

usage:
ocr sample X
pops up a window, user draws an example of an X, user doubleclicks and the X is saved for later
ocr train
builds a neural net for each letter type, trains each of them on the samples until they predict perfectly
ocr test
pops up a window, user draws a letter and doubleclicks, the program tries to guess which letter was drawn
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <QtGui>
#include <math.h>
#include <QWidget>
#include <stdlib.h>
#include <cstdlib>

#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif

//graphics defs here
#include "ocr.h"
//neural network is defined here.  assumes neural.cpp is in the same directory too.
#include "neural.h"

using namespace std;
int* getSquares();

//some global vars:
//squares wide
const int GRIDWIDTH = 10;
//squares tall
const int GRIDHEIGHT = 20;
//the window is this many pixels wide
const int SCREENWIDTH = 400;
//and pixels tall
const int SCREENHEIGHT = 400;
//an array of clickable grid squares
Square* square[GRIDWIDTH][GRIDHEIGHT];
//the window background
QGraphicsScene* thescene;
OCRView* view;

//keep track of the command line input
//operation is "sample" "train" or "test"
//symbol is the letter entered when the operation is "sample"
string operation, symbol;
const int numLetters = 3;
Perceptron* perceptrons[numLetters];
char letters[numLetters];

//called immediately on "ocr train"
//reads the images in ocrdata.txt, builds a set of neural nets, trains them, and saves the weights to perceptron.txt
void train()
{
	//read the images from file ocrdata.txt
	ifstream datafile;	//file object
	string line;		//lines will go here
	datafile.open("ocrdata.txt");	//open the file
	//stop the program if the file isn't found
	if (!datafile.is_open())
	{
		cout << "Couldn't open ocrdata.txt" << endl;
		return;
	}
	int linecount = 0;	//keep track of how many samples are in the file
	//go through the file and just count the number of samples
	while (getline(datafile, line))
	{
		linecount++;
	}
	//close the file.  we'll reopen it in a moment.
	datafile.close();

	//make an array to hold the samples
	int** sample_input = new int*[linecount];
	for (int i = 0; i < linecount; i++)
		sample_input[i] = new int[GRIDWIDTH*GRIDHEIGHT];
	//make another array to hold the output letter for each sample
	char* sample_output = new char[linecount];
	//reopen the file
	datafile.open("ocrdata.txt");
	//for each sample,
	for (int i = 0; i < linecount; i++)
	{
		//read it from the file
		getline(datafile, line);
		//the first character is the output letter
		sample_output[i] = line[0];
		//then a space, then a 1 or 0 for each square on the screen
		for (int j = 0; j < GRIDWIDTH*GRIDHEIGHT - 1; j++)
		{
			sample_input[i][j] = line[j + 2] == '1' ? 1 : 0;
		}
	}
	//now we're done with ocrdata.txt
	datafile.close();

	//TODO: MAKE SOME NEURAL NETS AND TRAIN THEM HERE, THEN SAVE THE WEIGHTS TO perceptron.txt
	for (int i = 0; i < numLetters; i++)
		perceptrons[i] = new Perceptron(GRIDHEIGHT*GRIDWIDTH);
	letters[0] = 'A';
	letters[1] = 'D';
	letters[2] = 'e';

	bool isCorrect = false;
	int numCorrect = 0;
	while (!isCorrect && numCorrect < 100)//trains for one letter
	{
		isCorrect = true;//assume it's true
		for (int i = 0; i < linecount; i++)//go through each line
		{
			for (int j = 0; j < numLetters; j++)
			{
				sample_output[i];
				letters[j];
				if (sample_output[i] == letters[j])//if this line has the letter that the perceptron is trained for
				{
					if (!perceptrons[j]->train(sample_input[i], 1))//if we get a false, restart
						isCorrect = false;
				}
				else
				{
					if (!perceptrons[j]->train(sample_input[i], 0))
						isCorrect = false;
				}
			}
		}
		if (isCorrect)
		{
			numCorrect++;
		}
	}

	for (int i = 0; i < linecount; i++)//prints out each guess
	{
		for (int j = 0; j < numLetters; j++)
		{
			qDebug() << sample_output[i] << ": " << letters[j] << ":" << perceptrons[j]->getPrediction(sample_input[i]) << endl;
		}
	}

	////save the perceptron's weights, saved in output followed by hidden
	ofstream perceptronFile;
	perceptronFile.open("perceptron.txt", ios::out | ios::app);
	for (int k = 0; k < numLetters; k++)
	{
		for (int i = 0; i < GRIDHEIGHT*GRIDWIDTH + 1; i++)
		{
			perceptronFile << perceptrons[k]->outputweight[i];
			perceptronFile << "\n";
		}
		for (int i = 0; i < GRIDHEIGHT*GRIDWIDTH; i++)
		{
			for (int j = 0; j < GRIDHEIGHT*GRIDWIDTH + 1; j++)
			{
				perceptronFile << perceptrons[k]->hiddenweight[i][j];
				perceptronFile << "\n";
			}
		}
	}
	perceptronFile << endl;
	perceptronFile.close();
}


//called on "ocr test", after the user draws and double-clicks the mouse
void test()	//TODO: MAKE SOME NEURAL NETS, READ THE WEIGHTS FROM A FILE perceptron.txt, USE THE NEURAL NETS TO IDENTIFY THE LETTER
{
	ifstream perceptronFile; //grab the file
	string line;//empty line
	perceptronFile.open("perceptron.txt");//open it

	for (int i = 0; i < numLetters; i++)
		perceptrons[i] = new Perceptron(GRIDHEIGHT*GRIDWIDTH);
	letters[0] = 'A';
	letters[1] = 'D';
	letters[2] = 'e';
	for (int k = 0; k < numLetters; k++)
	{
		for (int i = 0; i < GRIDHEIGHT*GRIDWIDTH; i++)//set it up with all of the correct output weights
		{
			getline(perceptronFile, line);
			perceptrons[k]->outputweight[i] = std::atof(line.c_str());
			line = "";
		}

		for (int i = 0; i < GRIDHEIGHT*GRIDWIDTH; i++)//set it up with all of the correct hidden weights
		{
			for (int j = 0; j < GRIDHEIGHT*GRIDWIDTH + 1; j++)
			{
				getline(perceptronFile, line);
				perceptrons[k]->hiddenweight[i][j] = std::atof(line.c_str());
				line = "";

			}
		}
	}
	perceptronFile.close();

	//take in the input from the screen
	int* s = getSquares();
	float highest = 99999;
	int chosenIndex = 0;
	for (int i = 0; i < numLetters; i++)
	{
		double hello = perceptrons[i]->getRawPrediction(s);
		qDebug() << letters[i] << ":" << hello;
		if (perceptrons[i]->getRawPrediction(s) < highest)
		{

			chosenIndex = i;
			highest = perceptrons[i]->getRawPrediction(s);
		}
	}
	qDebug() << letters[chosenIndex];
}

//read the contents of the grid and save them to the end of ocrdata.txt
void saveSample()
{
	ofstream datafile;
	datafile.open("ocrdata.txt", ios::out | ios::app);
	datafile << symbol << " ";
	int* s = getSquares();
	for (int i = 0; i < GRIDWIDTH*GRIDHEIGHT; i++)
		datafile << s[i];
	datafile << endl;
	datafile.close();
	cout << "Wrote sample to ocrdata.txt" << endl;
}

//determines what function is called when the user double clicks the window, based on the command line input
void doOperation()
{
	if (operation.compare("sample") == 0)
		saveSample();
	else if (operation.compare("train") == 0)
		train();
	else if (operation.compare("test") == 0)
		test();
}

//returns an integer array of 0s and 1s that represents what the user drew on the window
int* getSquares()
{
	int* s = new int[GRIDWIDTH*GRIDHEIGHT];
	for (int i = 0; i < GRIDWIDTH; i++)
		for (int j = 0; j < GRIDHEIGHT; j++)
			s[i*GRIDHEIGHT + j] = square[i][j]->selected ? 1 : 0;
	return s;
}

//constructor for a grid square
Square::Square(int x, int y)
{
	xpos = x; ypos = y;
	xcoor1 = x*SCREENWIDTH / GRIDWIDTH;
	ycoor1 = y*SCREENHEIGHT / GRIDHEIGHT;
	xcoor2 = (x + 1)*SCREENWIDTH / GRIDWIDTH;
	ycoor2 = (y + 1)*SCREENHEIGHT / GRIDHEIGHT;
	selected = FALSE;
}

//location of the square on the screen, used for rendering
QRectF Square::boundingRect() const
{
	return QRectF(xcoor1, ycoor1, xcoor2 - xcoor1, ycoor2 - ycoor1);
}

//called when the user selects the square
void Square::click()
{
	selected = TRUE;
	update();
}

//render the square on the screen
void Square::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
	QColor color;
	if (selected)
	{
		//if the user clicked it, draw it purple
		color.setBlue(100); color.setRed(100); color.setGreen(0);
	}
	else
	{
		//otherwise draw it white
		color.setBlue(255); color.setRed(255); color.setGreen(255);
	}
	painter->setBrush(color);
	painter->drawRect(xcoor1, ycoor1, xcoor2 - xcoor1, ycoor2 - ycoor1);
}

OCRView::OCRView(QGraphicsScene *scene, QWidget* parent) :QGraphicsView(scene, parent)
{
	//the mouse is initially assumed to be unpressed
	isPressed = FALSE;
}

//the mouse has been pressed or dragged
//figure out which grid square was selected and call it
void squareClickEvent(QMouseEvent *event)
{
	int x = (event->x()*GRIDWIDTH / SCREENWIDTH);
	int y = (event->y()*GRIDHEIGHT / SCREENHEIGHT);
	if (x < 0 || y < 0 || x >= GRIDWIDTH || y >= GRIDHEIGHT) return;
	square[x][y]->click();
}

//if the mouse button is down and the user moves over a square, select it
void OCRView::mouseMoveEvent(QMouseEvent *event)
{
	if (!isPressed) return;
	squareClickEvent(event);
}

//if the user presses the mouse on a square, select it
void OCRView::mousePressEvent(QMouseEvent *event)
{
	isPressed = TRUE;
	squareClickEvent(event);
}

//the mouse is released, stop selecting squares
void OCRView::mouseReleaseEvent(QMouseEvent *event)
{
	isPressed = FALSE;
}

//double click means that we're done drawing
void OCRView::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::RightButton)
	{
		// a right-button double click just exits the program
		exit(0);
	}
	if (event->button() == Qt::LeftButton)
	{
		//a left-button double click saves the sample or tests, depending on the command line parameters
		doOperation();
		exit(0);
	}
}

//program starts here
int main(int argc, char **argv)
{
	argv[0] = argv[0];
	argv[1] = argv[1];
	//save the command line arguments
	if (argc >= 2)
		operation = argv[1];
	if (argc == 3)
		symbol = argv[2];
	else
		symbol = " ";
	//if the user didn't give any, print out some help and stop
	if (argc == 1)
	{
		cout << "Usage: " << endl;
		cout << " ocr sample A" << endl;
		cout << " ocr train" << endl;
		cout << " ocr test" << endl;
		exit(0);
	}
	//seed the random number generator
	qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));

	//we don't need to make a window if the user selects "train"
	if (operation.compare("train") == 0)
	{
		train();
		exit(0);
	}

	//make a window
	QApplication app(argc, argv);

	thescene = new QGraphicsScene();
	thescene->setSceneRect(0, 0, SCREENWIDTH + 100, SCREENHEIGHT + 100);

	//add in all the squares
	for (int i = 0; i < GRIDWIDTH; i++)
	{
		for (int j = 0; j < GRIDHEIGHT; j++)
		{
			square[i][j] = new Square(i, j);
			thescene->addItem(square[i][j]);
		}
	}

	view = new OCRView(thescene);
	view->setWindowTitle("Neural OCR");
	view->resize(SCREENWIDTH + 100, SCREENHEIGHT + 100);
	view->show();
	view->setMouseTracking(true);

	//make the window visible.  we're done with setup.
	return app.exec();
}
