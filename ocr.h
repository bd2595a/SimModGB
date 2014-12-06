/*Defines some qt graphical objects
Michael Black, 10/2014*/

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QTimer>
#include <QWidget>

class QGraphicsScene;


//A window background that is responsive to mouse clicks
class OCRView : public QGraphicsView
{
	Q_OBJECT
public:
	OCRView(QGraphicsScene*, QWidget*x=  NULL);
	//is the mouse putton currently down?
	bool isPressed;
protected:
	//called when the mouse is pressed
	void mousePressEvent(QMouseEvent *event);
	//called on a double click
	void mouseDoubleClickEvent(QMouseEvent *event);
	//called when the mouse button is released
	void mouseReleaseEvent(QMouseEvent *event);
	//called whenever the mouse is moved
        void mouseMoveEvent(QMouseEvent *event);
};

//a single grid square
class Square : public QGraphicsItem
{
public:
	//new Square(3,5) defines a square 3 over and 5 down
	Square(int,int);
	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	//called if the mouse is pressed on the square or dragged across the square
	void click();
	//the square's grid position and actual pixel position, both start and end
	int xpos,ypos,xcoor1,ycoor1,xcoor2,ycoor2;
	//TRUE if the square has been clicked and darkened, FALSE initially
	bool selected;
};
