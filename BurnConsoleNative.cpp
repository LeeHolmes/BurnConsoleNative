/*
###############################################################################
## BurnConsoleNative.cpp
##
## Create a fire effect in C, using the Console text buffer as the 
## rendering surface.
##
## Great overview of the fire effect algorithm here: 
## http://freespace.virgin.net/hugo.elias/models/m_fire.htm
##
###############################################################################
*/

#include "stdafx.h"
#include "windows.h"
#include "time.h"
#include "conio.h"

#define WINDOW_WIDTH 70
#define WINDOW_HEIGHT 61

HANDLE wHnd;    // Handle to write to the console.
HANDLE rHnd;    // Handle to read from the console.
PCHAR_INFO palette = new CHAR_INFO[256];
int screenBuffer[WINDOW_WIDTH * WINDOW_HEIGHT];
int workingBuffer[WINDOW_WIDTH * WINDOW_HEIGHT]; 

void generatePalette(void);
void displayPalette(void);
void updateBuffer(void);
void updateScreen(void);

int _tmain(int argc, _TCHAR* argv[])
{
	// Prepare the random number generator
	srand ( (unsigned int) time(NULL) );

	// Set up the handles for reading/writing:
    wHnd = GetStdHandle(STD_OUTPUT_HANDLE);
    rHnd = GetStdHandle(STD_INPUT_HANDLE);

	// Set up the required window size:
    SMALL_RECT windowSize = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SetConsoleWindowInfo(wHnd, TRUE, &windowSize);
    COORD bufferSize = {WINDOW_WIDTH, WINDOW_HEIGHT};
    SetConsoleScreenBufferSize(wHnd, bufferSize);

	generatePalette();
	//displayPalette();

	int frameCount = 0;
	time_t startTime = time(NULL);
	
	while(! _kbhit())
	{
		updateBuffer();
		updateScreen();
		frameCount++;
	}
	time_t endTime = time(NULL);
	double seconds = (double) endTime - startTime;

	printf("%f frames per second\n", ((double) frameCount / seconds));
	_getch();
		
	return 0;
}

// Generates a palette of 256 colours.  We create every combination of 
// foreground colour, background colour, and dithering character, and then
// order them by their visual intensity.
//
// The visual intensity of a colour can be expressed by the NTSC luminance 
// formula.  That formula depicts the apparent brightness of a colour based on 
// our eyes' sensitivity to different wavelengths that compose that colour.
// http://en.wikipedia.org/wiki/Luminance_%28video%29
void generatePalette()
{
	RtlZeroMemory(palette, 256 * sizeof(CHAR_INFO));

    // Rather than a simple red fire, we'll introduce oranges and yellows 
    // by including Yellow as one of the base colours 
	char* colours[] = { "Yellow", "Red", "DarkRed", "Black" };
	WORD fgAttributes[] = { 14, 12, 4, 0 };
	WORD bgAttributes[] = { 14 << 4, 12 << 4, 4 << 4, 0 };


	// The apparent intensities of our four primary colours. 
    // However, the formula under-represents the intensity of our straight 
    // red colour, so we artificially inflate it. 
	double luminances[] = { 225.93,106.245,38.272,0 };
	double maxBrightness = luminances[0];

    // The four characters that we use to dither with, along with the  
    // percentage of the foreground colour that they show
	LPWSTR dithering = L"█▓▒░";
	double ditherFactor[] = { 1, 0.75, 0.5, 0.25 };

	// Cycle through each foreground, background, and dither character 
    // combination.  For each combination, find the apparent intensity of the  
    // foreground, and the apparent intensity of the background.  Finally, 
    // weight the contribution of each based on how much of each colour the 
    // dithering character shows. 
    // This provides an intensity range between zero and some maximum. 
    // For each apparent intensity, we store the colours and characters 
    // that create that intensity.
	int numColours = (sizeof(colours) / sizeof(colours[0]));
    for(int fgColour = 0; fgColour < numColours; fgColour++) 
    { 
        for(int bgColour = 0; bgColour < numColours; bgColour++) 
        { 
            for(int ditherCharacter = 0;  
                ditherCharacter < (sizeof(dithering) / sizeof(dithering[0]));  
                ditherCharacter++) 
            { 
                double apparentBrightness = 
                    luminances[fgColour] * ditherFactor[ditherCharacter] + 
                    luminances[bgColour] * 
                        (1 - ditherFactor[ditherCharacter]);
				double scaledBrightness = apparentBrightness / maxBrightness;
				int arrayIndex = (int) (scaledBrightness * 255);
                
				CHAR_INFO newChar;
				newChar.Char.UnicodeChar = dithering[ditherCharacter];
				newChar.Attributes = fgAttributes[fgColour] | bgAttributes[bgColour];
				palette[arrayIndex] = newChar;
            } 
       }
    }

	CHAR_INFO paletteBrush = palette[255];
	for(int paletteIndex = 255; paletteIndex >= 0; paletteIndex--)
	{
		if(palette[paletteIndex].Char.UnicodeChar != NULL)
			paletteBrush = palette[paletteIndex];
		else
			palette[paletteIndex] = paletteBrush;
	}
}

void displayPalette(void)
{
	for(int paletteIndex = 254; paletteIndex >= 0; paletteIndex--)
	{
		SetConsoleTextAttribute(wHnd, palette[paletteIndex].Attributes);
		WriteConsole(wHnd, & palette[paletteIndex].Char.UnicodeChar, 1, NULL, NULL);
	}
}

// Update a back-buffer to hold all of the information we want to display on
// the screen.  To do this, we first re-generate the fire pixels on the bottom 
// row.  With that done, we visit every pixel in the screen buffer, and figure
// out the average heat of its neighbors.  Once we have that average, we move
// that average heat one pixel up.
void updateBuffer(void)
{
    // Start fire on the last row of the screen buffer 
    for(int column = 0; column < WINDOW_WIDTH; column++) 
    { 
        // There is an 80% chance that a pixel on the bottom row will 
        // start new fire. 
        if(rand() % 10 >= 2) 
        { 
            // The chosen pixel gets a random amount of heat.  This gives 
            // us a lot of nice colour variation. 
            screenBuffer[(WINDOW_HEIGHT - 2) * (WINDOW_WIDTH) + column] = rand() % 255;
        }
    }

	// Clone the screen buffer
	memcpy(workingBuffer, screenBuffer, WINDOW_HEIGHT * WINDOW_WIDTH * sizeof(int));

	// Propigate the fire
    for(int row = 1; row < (WINDOW_HEIGHT - 1); row++) 
    { 
        for(int column = 1; column < (WINDOW_WIDTH - 1); column++) 
        { 
            // BaseOffset is the location of the current pixel 
            int baseOffset = (WINDOW_WIDTH * row) + column; 
     
            // Get the average colour from the four pixels surrounding 
            // the current pixel 
            double colour = screenBuffer[baseOffset];
            colour += screenBuffer[baseOffset - 1];
            colour += screenBuffer[baseOffset + 1];
            colour += screenBuffer[baseOffset + WINDOW_WIDTH];
            colour /= 4.0;

            // Cool it off a little.  We apply uneven cooling, otherwise 
            // the cool dark red tends to stretch up for too long. 
            if(colour > 70) { colour -= 1; } 
            if(colour <= 70) { colour -= 3; } 
            if(colour < 20) { colour -= 1; } 
            if(colour < 0) { colour = 0; } 

            // Store the result into the previous row -- that is, one buffer  
			// cell up. 
            workingBuffer[baseOffset - WINDOW_WIDTH] = (int) colour; 
        } 
    } 
}

// Take the contents of our working buffer and blit it to the screen
// We do this in one highly-efficent step (the SetBufferContents) so that
// users don't see each individial pixel get updated.
void updateScreen(void)
{
	SMALL_RECT copyRegion; 
    CHAR_INFO chiBuffer[WINDOW_WIDTH * WINDOW_HEIGHT]; 
    COORD coordBufSize; 
    COORD coordBufCoord; 

	// Set the copy region. 
    copyRegion.Top = 0;
    copyRegion.Left = 0;
    copyRegion.Bottom = WINDOW_HEIGHT - 1;
    copyRegion.Right = WINDOW_WIDTH - 1;
 
    coordBufSize.Y = WINDOW_HEIGHT;
    coordBufSize.X = WINDOW_WIDTH;
 
    // The top left destination cell of the temporary buffer is 
    // row 0, col 0. 
    coordBufCoord.X = 0; 
    coordBufCoord.Y = 0; 
 
    // Copy the block from the screen buffer to the temp. buffer. 
    ReadConsoleOutput( 
       wHnd,          // screen buffer to read from 
       chiBuffer,      // buffer to copy into 
       coordBufSize,   // col-row size of chiBuffer 
       coordBufCoord,  // top left dest. cell in chiBuffer 
       &copyRegion); // screen buffer source rectangle 

	for(int row = 0; row < WINDOW_HEIGHT; row++)
	{
		for(int column = 0; column < WINDOW_WIDTH; column++)
		{
			chiBuffer[row * WINDOW_WIDTH + column] =
				palette[workingBuffer[row * WINDOW_WIDTH + column]];
		}
	}

 	WriteConsoleOutput( 
        wHnd, // screen buffer to write to 
        chiBuffer,        // buffer to copy from 
        coordBufSize,     // col-row size of chiBuffer 
        coordBufCoord,    // top left src cell in chiBuffer 
        &copyRegion);  // dest. screen buffer rectangle 


	// And update our internal representation of the screen
	memcpy(screenBuffer, workingBuffer, WINDOW_HEIGHT * WINDOW_WIDTH * sizeof(int));
}