using namespace std;

#include <fstream>
#include <QDebug>
#include "Z80.h"
#include "screen.h"

char* rom;// = { 0x06, 0x06, 0x3e, 0x00, 0x80, 0x05, 0xc2, 0x04, 0x00, 0x76 };
unsigned char graphicsRAM[8192];
int palette[4];
int tileset, tilemap, scrollx, scrolly;
extern QApplication* app;
int HBLANK = 0, VBLANK = 1, SPRITE = 2, VRAM = 3;
unsigned char workingRAM[0x2000];
unsigned char page0RAM[0x80];
int line = 0, cmpline = 0, videostate = 0, keyboardColumn = 0, horizontal = 0;
int gpuMode = HBLANK;
int romOffset = 0x4000;
long totalInstructions = 0;

unsigned char getKey() { return 0xf; }
void setRomMode(int address, unsigned char b) { }
void setControlByte(unsigned char b) {
	tilemap = (b & 8) != 0 ? 1 : 0;

	tileset = (b & 16) != 0 ? 1 : 0;
}
void setPalette(unsigned char b) {
	palette[0] = b & 3; palette[1] = (b >> 2) & 3; palette[2] = (b >> 4) & 3; palette[3] = (b >> 6) & 3;
}

unsigned char getVideoState() {
	int by = 0;

	if (line == cmpline) by |= 4;

	if (gpuMode == VBLANK) by |= 1;

	if (gpuMode == SPRITE) by |= 2;

	if (gpuMode == VRAM) by |= 3;

	return (unsigned char)((by | (videostate & 0xf8)) & 0xff);
}

unsigned char memoryread(int address)
{
	if (address >= 0 && address <= 0x3FFF)
		return rom[address];
	else if (address >= 0x4000 && address <= 0x7FFF)
		return rom[romOffset + address % 0x4000];
	else if (address >= 0x8000 && address <= 0x9FFF)
		return graphicsRAM[address % 0x2000];
	else if (address >= 0xC000 && address <= 0xDFFF)
		return workingRAM[address % 0x2000];
	else if (address >= 0xFF80 && address <= 0xFFFF)
		return page0RAM[address % 0x80];
	else if (address == 0xFF00)
		return getKey();
	else if (address == 0xFF41)
		return getVideoState();
	else if (address == 0xFF42)
		return scrolly;
	else if (address == 0xFF43)
		return scrollx;
	else if (address == 0xFF44)
		return line;
	else if (address == 0xFF45)
		return cmpline;
	else
		return 0;
	//return rom[address];
}
void memorywrite(int address, unsigned char value)
{
	if (address >= 0 && address <= 0x3FFF)
		setRomMode(address, value);
	else if (address >= 0x4000 && address <= 0x7FFF)
		qDebug() << value << " is outside of memorywrite spefications. (x4000-x7FFF)" << endl;
	else if (address >= 0x8000 && address <= 0x9FFF)
		graphicsRAM[address % 0x2000] = value;
	else if (address >= 0xC000 && address <= 0xDFFF)
		workingRAM[address % 0x2000] = value;
	else if (address == 0xFF00)
		keyboardColumn = value;
	else if (address == 0xFF40)
		setControlByte(value);
	else if (address == 0xFF41)
		videostate = value;
	else if (address == 0xFF42)
		scrolly = value;
	else if (address == 0xFF43)
		scrollx = value;
	else if (address == 0xFF44)
		line = value;
	else if (address == 0xFF45)
		cmpline = value;
	else if (address == 0xFF47)
		setPalette(value);
	else if (address >= 0xFF80 && address <= 0xFFFF)
		page0RAM[address % 0x80] = value;
	else
		qDebug() << value << " is outside of memorywrite spefications." << endl;
}

void renderScreen(){
	for (int row = 0; row < 144; row++)
	{
		for (int column = 0; column < 160; column++)
		{
			//apply scroll
			//SWAPPED X and Y
			int y = row, x = column;

			// x = (x + scrollx)&255;
			// y = (y + scrolly)&255;
			//THIS ERRORS FOR SOME REASON Bus error: 10 (MOVED DOWN!)

			//determine which tile pixel belongs to
			// int tilex = x/8, tiley = y/8;
			int tilex = ((x + scrollx) & 255) / 8, tiley = ((y + scrolly) & 255) / 8;
			//find tileposition in 1D array
			int tileposition = tiley * 32 + tilex;

			int tileindex, tileaddress;
			if (tileset == 1){ //tilemap1
				tileindex = graphicsRAM[0x1c00 + tileposition];
				tileaddress = tileindex * 16;
			}
			else { //tilemap0
				tileindex = graphicsRAM[0x1800 + tileposition];
				if (tileindex >= 128)
					tileindex -= 256;
				tileaddress = tileindex * 16 + 0x1000;
			}

			//get which pixel is in the tile
			int xoffset = x % 8, yoffset = y % 8; //should use &
			//get the two bytes for each row in tile
			int row0 = graphicsRAM[tileaddress + yoffset * 2];
			int row1 = graphicsRAM[tileaddress + yoffset * 2 + 1];

			//Binary math to get binary indexed color info across both bytes
			int row0shifted = row0 >> (7 - xoffset);
			int row0capturepixel = row0shifted & 1;

			int row1shifted = row1 >> (7 - xoffset);
			int row1capturepixel = row1shifted & 1;

			//combine byte info to get color
			int pixel = row1capturepixel * 2 + row0capturepixel;
			//get color based on palette
			int color = palette[pixel];

			updateSquare(x, y, color);
		}
	}
	onFrame();
}
//program starts here
int main(int argc, char **argv)
{
	setup(argc, argv);//sets up the window
	ifstream romfile("TETRIS.GB", ios::in | ios::binary | ios::ate);//read in instructions from gameboy
	streampos size = romfile.tellg();
	rom = new char[size];
	int romSize = size;
	romfile.seekg(0, ios::beg);
	romfile.read(rom, size);
	romfile.close();

	Z80* z80 = new Z80(memoryread, memorywrite);//create the z80
	z80->reset();//reset it
	while (true)//do all of its instructions
	{
		if (!z80->halted)
			z80->doInstruction();
		if (z80->interrupt_deferred > 0)//handle check for interrupts
		{
			z80->interrupt_deferred--;
			if (z80->interrupt_deferred == 1)
			{
				z80->interrupt_deferred = 0;
				z80->FLAG_I = 1;
			}
		}
		z80->checkForInterrupts();
		horizontal = (int)((totalInstructions + 1) % 61);
		if (line >= 145)//set the gpu mode
			gpuMode = VBLANK;
		else if (horizontal <= 30)
			gpuMode = HBLANK;
		else if (horizontal >= 31 && horizontal <= 40)
			gpuMode = SPRITE;
		else
			gpuMode = VRAM;

		if (horizontal == 0)
		{
			line++;
			if (line == 144)
				z80->throwInterrupt(1);//we reached VBLANK state
			if (line % 153 == cmpline && videostate & 0x40 != 0)
				z80->throwInterrupt(2);
			else if (line == 153)
			{
				line = 0;
				renderScreen();
				onFrame();
			}
		}
		totalInstructions++;
	}
	return 0;
}