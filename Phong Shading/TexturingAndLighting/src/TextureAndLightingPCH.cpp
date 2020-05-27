#include <TextureAndLightingPCH.h>

void drawStrokeText(char*string, int x, int y, int z)
{
	char *c;
	glPushMatrix();
	glTranslatef(x, y, z);
	glScalef(0.3f,0.3f,z);

	for (c = string; *c != '\0'; c++) {
		glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
		//glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, *c);
	}
	glPopMatrix();
}