#include <iostream>
#include "Z80.h"
#include <fstream>
#include "screen.h"
#include <QApplication>

using namespace std;

char* rom;
unsigned char graphicsRAM[8192];
int palette[4];
int tileset, tilemap, scrollx, scrolly;
int HBLANK = 0, VBLANK = 1, SPRITE = 2, VRAM = 3;
unsigned char workingRAM[0x2000];
unsigned char page0RAM[0x80];

int line = 0, cmpline = 0, videostate = 0, keyboardColumn = 0, horizontal = 0;
int gpuMode = HBLANK;
int romOffset = 0x4000;
long totalInstructions = 0;

void updateSquare(int, int, int);
void onFrame();
unsigned char getVideoState();
void setPalette(unsigned char);
void setControlByte(unsigned char);
void setRomMode(int, unsigned char);
unsigned char getKey();

unsigned char memoryRead(int address){
	if (address >= 0 && address <= 0x3FFF){
		return rom[address];
	}
	else if (address >= 0x4000 && address <= 0x7FFF){
		return rom[romOffset + address % 0x4000];
	}
	else if (address >= 0x8000 && address <= 0x9FFF){
		return graphicsRAM[address % 0x2000];
	}
	else if (address >= 0xC000 && address <= 0xDFFF){
		return workingRAM[address % 0x2000];
	}
	else if (address >= 0xFF80 && address <= 0xFFFF){
		return page0RAM[address % 0x80];
	}
	else if (address == 0xFF00){
		return getKey();
	}
	else if (address == 0xFF41){
		return getVideoState();
	}
	else if (address == 0xFF42){
		return scrolly;
	}
	else if (address == 0xFF43){
		return scrollx;
	}
	else if (address == 0xFF44){
		return line;
	}
	else if (address = 0xFF45){
		return cmpline;
	}
	else{
		return 0;
	}
}

void memoryWrite(int address, unsigned char b){
	if (address >= 0 && address <= 0x3FFF){
		setRomMode(address, b);
	}
	else if (address >= 0x8000 && address <= 0x9FFF){
		graphicsRAM[address % 0x2000] = b;
	}
	else if (address >= 0xC000 && address <= 0xDFFF){
		workingRAM[address % 0x2000] = b;
	}
	else if (address >= 0xFF80 && address <= 0xFFFF){
		page0RAM[address % 0x80] = b;
	}
	else if (address == 0xFF00){
		keyboardColumn = b;
	}
	else if (address == 0xFF40){
		setControlByte(b);
	}
	else if (address == 0xFF41){
		videostate = b;
	}
	else if (address == 0xFF42){
		scrolly = b;
	}
	else if (address == 0xFF43){
		scrollx = b;
	}
	else if (address == 0xFF44){
		line = b;
	}
	else if (address == 0xFF45){
		cmpline = b;
	}
	else if (address == 0xFF47){
		setPalette(b);
	}
}

unsigned char getKey(){
	return 0xf;
}

void setRomMode(int address, unsigned char b){

}

void setControlByte(unsigned char b){
	tilemap = (b & 8) != 0 ? 1 : 0;
	tileset = (b & 16) != 0 ? 1 : 0;
}

void setPalette(unsigned char b){
	palette[0] = b & 3;
	palette[1] = (b >> 2) & 3;
	palette[2] = (b >> 4) & 3;
	palette[3] = (b >> 6) & 3;
}

unsigned char getVideoState(){
	int by = 0;
	if (line == cmpline){
		by |= 4;
	}
	if (gpuMode == VBLANK){
		by |= 1;
	}
	if (gpuMode == SPRITE){
		by |= 2;
	}
	if (gpuMode == VRAM){
		by |= 3;
	}
	return (unsigned char)((by | (videostate & 0xf8)) & 0xff);
}



void renderScreen(){
	for (int row = 0; row < 144; row++){
		for (int column = 0; column < 160; column++){
			int x = column;
			int y = row;
			int tilex = ((x + scrollx) & 255) / 8;
			int tiley = ((y + scrolly) & 255) / 8;
			int tileposition = tiley * 32 + tilex;
			int tileaddress = 0;
			int tileindex = 0;
			if (tilemap == 0){
				tileindex = graphicsRAM[0x1800 + tileposition];
				if (tileindex >= 128){
					tileindex -= 256;
				}
				tileaddress = tileindex * 16 + 0x1000;
			}
			else if (tilemap == 1){
				tileindex = graphicsRAM[0x1c00 + tileposition];
				tileaddress = tileindex * 16;
			}
			int xoffset = x % 8;
			int yoffset = y % 8;
			int row0 = graphicsRAM[tileaddress + yoffset * 2];
			int row1 = graphicsRAM[tileaddress + yoffset * 2 + 1];
			int row0shifted = row0 >> (7 - xoffset);
			int row0capturepixel = row0shifted & 1;
			int row1shifted = row1 >> (7 - xoffset);
			int row1capturepixel = row1shifted & 1;
			int pixel = row1capturepixel * 2 + row0capturepixel;
			int color = palette[pixel];
			updateSquare(x, y, color);
		}
	}
	onFrame();
}

extern QApplication* app;
int main(int argc, char** argv){
	setup(argc, argv);
	ifstream romfile;
	romfile.open("opus5.gb", ios::in | ios::binary | ios::ate);
	streampos size = romfile.tellg();

	rom = new char[size];

	int romSize = size;

	romfile.seekg(0, ios::beg);

	romfile.read(rom, size);

	romfile.close();
	Z80* z80 = new Z80(memoryRead, memoryWrite);
	z80->reset();
	while (true){
		if (!z80->halted){
			//	cout << "z80 not halted, doing instruction" << endl;
			z80->doInstruction();
			//cout << "here" << endl;
		}
		if (z80->interrupt_deferred > 0){
			//cout << "interrupt deferred, = " << z80->interrupt_deferred << endl;
			z80->interrupt_deferred--;
			if (z80->interrupt_deferred == 1){
				z80->interrupt_deferred = 0;
				z80->FLAG_I = 1;
				//cout << "interrupt flag set" << endl;
			}
		}
		//cout << "now checking for interrupts" << endl;
		z80->checkForInterrupts();
		horizontal = (int)((totalInstructions + 1) % 61);
		//cout << "horizontal calculated, = " << horizontal << endl;
		if (line >= 145){
			//cout << "VBLANK" << endl;
			gpuMode = VBLANK;
		}
		else if (horizontal <= 30){
			//cout << "HBLANK" << endl;
			gpuMode = HBLANK;
		}
		else if (horizontal >= 31 && horizontal <= 40){
			gpuMode = SPRITE;
			//cout << "SPRITE" << endl;
		}
		else{
			//cout << "VRAM" << endl;
			gpuMode = VRAM;
		}

		if (horizontal == 0){
			//cout << "horizontal = 0" << endl;
			line++;
			if (line == 144){
				z80->throwInterrupt(1);
				//cout << "line = 144, interrupt thrown" << endl;
			}
			if (line % 153 == cmpline && (videostate & 0x40) != 0){
				z80->throwInterrupt(2);
				//cout << "interrupt 2 thrown" << endl;
			}
			if (line == 153){
				line = 0;
				renderScreen();
				//cout << "line set back to 0, rendering screen" << endl;
			}
		}
		totalInstructions++;
	}
	cout << "PC: " << z80->PC << ", instruction: " << z80->instruction << ", A: " << z80->A << ", B: " << z80->B << endl;


	//app->exec();
}
