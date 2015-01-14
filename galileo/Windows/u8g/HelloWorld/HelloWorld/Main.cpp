// Main.cpp : Defines the entry point for the console application.
//

// Include files
//
// This solution assumes that you have local clone of the u8glib and m2tklib repositories.
// You will need to add these directories to your Additional Include Directories property
//
// u8glib\cppsrc; 
// m2tklib\dev\u8glib; 
// u8glib\csrc; 
// m2tklib\cpp; 
// m2tklib\src;
//
#include "stdafx.h"
#include "arduino.h"
#include "M2tk.h"
#include "m2ghu8g.h"
#include "spi.h"
#include "U8glib.h"

U8GLIB_LM6059 u8g(8, 10, 9);

int _tmain(int argc, _TCHAR* argv[])
{
	return RunArduinoSketch();
}

M2_LABEL(hello_world_label, NULL, "Hello World");
M2tk m2(&hello_world_label, NULL, NULL, m2_gh_u8g_bfs);

void draw(void) {
	m2.draw();
}

void setup() {
	/* connect u8glib with m2tklib */
	m2_SetU8g(u8g.getU8g(), m2_u8g_box_icon);

	/* assign u8g font to index 0 */
	m2.setFont(0, u8g_font_7x13);
}

void loop() {
	u8g.firstPage();
	do {
		draw();
	} while (u8g.nextPage());
}