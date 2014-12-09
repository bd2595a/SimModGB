#include <iostream>
#include <fstream>
#include "z80.h"
#include "screen.h"
#include <QApplication>
using namespace std;
Z80* z80;
int color, tileset, tilemap, scrollx, scrolly, tileposition, tileaddress, tileindex, tilex, tiley, row0, row1, row0shifted, row0capturepixel, row1shifted, row1capturepixel, pixel, x, y, xoffset, yoffset;
int HBLANK = 0, VBLANK = 1, SPRITE = 2, VRAM = 3;
unsigned char workingRAM[0x2000];
unsigned char page0RAM[0x80];
int line = 0, cmpline = 0, videostate = 0, keyboardColumn = 0, horizontal = 0;
int gpuMode = HBLANK;
int romOffset = 0x4000;
long totalInstructions = 0;
unsigned char graphicsRAM[8192];
int palette[4];
char* rom;
unsigned char getKey()
{
	return 0xf;
}
void setRomMode(int address, unsigned char b)
{

}
void setControlByte(unsigned char b)
{
	tilemap = (b & 8) != 0 ? 1 : 0;

	tileset = (b & 16) != 0 ? 1 : 0;
}
void setPalette(unsigned char b)
{
	palette[0] = b & 3; palette[1] = (b >> 2) & 3; palette[2] = (b >> 4) & 3; palette[3] = (b >> 6) & 3;
}

unsigned char getVideoState()
{
	int by = 0;

	if (line == cmpline) by |= 4;

	if (gpuMode == VBLANK) by |= 1;

	if (gpuMode == SPRITE) by |= 2;

	if (gpuMode == VRAM) by |= 3;

	return (unsigned char)((by | (videostate & 0xf8)) & 0xff);
}

unsigned char memoryread(int address)
{
	//
	if (address >= 0 && address <= 0x3FFF)
	{
		return rom[address];
	}
	//
	else if (address >= 0x4000 && address <= 0x7FFF)
	{
		return rom[romOffset + address % 0x4000];
	}
	//
	else if (address >= 0x8000 && address <= 0x9FFF)
	{
		return graphicsRAM[address % 0x2000];
	}
	else if (address >= 0xC000 && address <= 0xDFFF)
	{
		return workingRAM[address % 0x2000];
	}
	else if (address >= 0xFF80 && address <= 0xFFFF)
	{
		return page0RAM[address % 0x80];
	}
	else if (address == 0xFF00)
	{
		return getKey();
	}
	else if (address == 0xFF41)
	{
		return getVideoState();
	}
	else if (address == 0xFF42)
	{
		return scrolly;
	}
	else if (address == 0xFF43)
	{
		return scrollx;
	}
	else if (address == 0xFF44)
	{
		return line;
	}
	else if (address == 0xFF45)
	{
		return cmpline;
	}
	else
	{
		return 0;
	}

}
void memoryWrite(int address, unsigned char b)
{
	if (address >= 0 && address <= 0x3FFF)
	{
		setRomMode(address, b);

	}
	//
	else if (address >= 0x4000 && address <= 0x7FFF)
	{

	}
	//
	else if (address >= 0x8000 && address <= 0x9FFF)
	{
		graphicsRAM[address % 0x2000] = b;
	}
	else if (address >= 0xC000 && address <= 0xDFFF)
	{
		workingRAM[address % 0x2000] = b;
	}
	else if (address >= 0xFF80 && address <= 0xFFFF)
	{
		page0RAM[address % 0x80] = b;
	}
	else if (address == 0xFF00)
	{
		keyboardColumn = b;
	}
	else if (address == 0xFF40)
	{
		setControlByte(b);
	}
	else if (address == 0xFF41)
	{
		videostate = b;
	}
	else if (address == 0xFF42)
	{
		scrolly = b;
	}
	else if (address == 0xFF43)
	{
		scrollx = b;
	}
	else if (address == 0xFF44)
	{
		line = b;
	}
	else if (address == 0xFF45)
	{
		cmpline = b;
	}
	else if (address == 0xFF47)
	{
		setPalette(b);
	}


}
extern QApplication* app;
void renderScreen()
{
	for (int i = 0; i<160; i++)
	{
		for (int j = 0; j<144; j++)
		{
			//y = (j + scrolly)&255;
			//x = (i + scrollx)&255;
			y = j;
			x = i;
			cout << "X: " << x << " Y: " << endl;
			cout << scrolly << endl;
			tilex = ((x + scrollx) & 255) / 8;
			tiley = ((y + scrolly) & 255) / 8;
			tileposition = tiley * 32 + tilex;
			if (tilemap == 0)
			{
				tileindex = graphicsRAM[0x1800 + tileposition];
			}
			else if (tilemap == 1)
			{
				tileindex = graphicsRAM[0x1c00 + tileposition];
			}
			if (tileset == 0)
			{
				if (tileindex >= 128)
				{
					tileindex = tileindex - 256;
				}
				tileaddress = tileindex * 16 + 0x1000;
			}
			else if (tileset == 1)
			{
				tileaddress = tileindex * 16;
			}

			xoffset = x % 8;
			yoffset = y % 8;
			row0 = graphicsRAM[tileaddress + yoffset * 2];
			row1 = graphicsRAM[tileaddress + yoffset * 2 + 1];
			row0shifted = row0 >> (7 - xoffset);
			row0capturepixel = row0shifted & 1;
			row1shifted = row1 >> (7 - xoffset);
			row1capturepixel = row1shifted & 1;
			pixel = row1capturepixel * 2 + row0capturepixel;
			color = palette[pixel];
			updateSquare(x, y, color);


		}

	}
	onFrame();

}

int main(int argc, char **argv)
{
	setup(argc, argv);


	//part 1
	std::ifstream romfile("ttt.gb", ios::in | ios::binary | ios::ate);
	streampos size = romfile.tellg();
	rom = new char[size];
	int romSize = size;
	romfile.seekg(0, ios::beg);
	romfile.read(rom, size);
	romfile.close();

	z80 = new Z80(memoryread, memoryWrite);
	z80->reset();
	while (true)
	{
		if (!z80->halted)
		{
			z80->checkForInterrupts();
		}
		if (z80->interrupt_deferred>0)
		{
			z80->interrupt_deferred--;
			if (z80->interrupt_deferred == 1)
			{
				z80->interrupt_deferred = 0;
				z80->FLAG_I = 1;
			}

		}

		horizontal = (int)((totalInstructions + 1) % 61);
		if (line >= 145)
		{
			gpuMode = VBLANK;
		}
		else if (horizontal <= 30)
		{
			gpuMode = HBLANK;
		}
		else if (horizontal >= 31 || horizontal <= 40)
		{
			gpuMode = SPRITE;
		}
		else
		{
			gpuMode = VRAM;
		}
		std::cout << "PC: " << z80->PC << ", A: " << z80->A << ", B: " << z80->B << std::endl;
		if (horizontal == 0)
		{
			line++;
			if (line == 144)
			{
				z80->throwInterrupt(1);
			}
			if (line % 153 == cmpline && (videostate & 0x40) != 0)
			{
				z80->throwInterrupt(2);
			}
			if (line == 153)
			{
				line = 0;
				renderScreen();
			}
		}
		
		totalInstructions++;
	}
	//part 2
	return 0;
}