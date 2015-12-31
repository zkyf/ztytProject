#define _USE_MATH_DEFINES

#ifndef GLUT_DISABLE_ATEXIT_HACK  
#define GLUT_DISABLE_ATEXIT_HACK 
#endif  

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <gl/glut.h>
#include <math.h>
#include <Windows.h>
#include "objin.h"
#include "resource.h"

#define PBSIZE (1024)

using namespace std;

typedef enum
{
	UP,
	DOWN,
	FOREWARD,
	BACKWARD,
	LEFT,
	RIGHT
} Direction;

static GLint mousex = 0, mousey = 0;
GLboolean  mouserdown = GL_FALSE;
GLboolean  mouseldown = GL_FALSE;
GLboolean  mousemdown = GL_FALSE;

/// Callback Functions
void display();
void reshape(int width, int height);
void keyboard(unsigned char key, int x, int y);
void mouse(GLint button, GLint action, GLint x, GLint y);
void motion(int x, int y);
void wheel(int button, int dir, int x, int y);

/// Control Functions
void moveeye(Direction dir, double step);

///OpenGL Related Functions
void loadobj(char* filename);
void generateEye(GLfloat eye[], GLfloat lookat[3]);
void generateEye(GLfloat eye[], GLfloat lookat[3], double step);
void updateView(int width, int height);
void draw(GLenum mode);
void drawCross();
void light();
void idle();
void debug_print_eye();
GLuint processpick(GLint n, GLuint pb[]);
void genlist();
void getFPS();;

class ljxObject
{
	public:
	string obj;
	string tga;
	GLMmodel model;
	bool havetext;
	bool chosen;
	double x, y, z;
	double angle;
	double sx;
	double sy;
	double sz;
	double osx, osy, osz;
	GLint lidwithtext;
	GLint lidwithouttext;
	GLMtexture texture;
	ljxObject() : havetext(false), x(0), y(0), z(0), angle(0), chosen(false) {}
	~ljxObject() {}
};

typedef vector<ljxObject> ObjectList;

const double dan = 1;

ObjectList objs;
int gwidth, gheight;
bool model = true;
bool ro = false;
bool follow = true;
bool withtext = true;
bool showlightpos = false;
bool fullscreen = true;
int objn = 0;
int chosen = -1;
GLint lid;
HWND hWnd;
HINSTANCE hInstance;
GLfloat eye[3] = { 0.0, 0.0, 8 };
GLfloat viewangle[2] = { -M_PI / 2, M_PI / 2 };
GLfloat light0col[3] = { 1.0, 1.0, 1.0 };
GLfloat light0pos[4] = { 10.0, 10.0, 10.0, 1.0 };
GLuint pb[PBSIZE];

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(640, 480);
	glutCreateWindow("[哈哈哈哈哈哈！]");

	if (argc < 2)
	{
		printf("No config file assigned!\n");
		system("pause");
		exit(1);
	}
	loadobj(argv[1]);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutIdleFunc(idle);
	glutMouseWheelFunc(wheel);

	if (glewInit() != GLEW_OK)
	{
		cout << "glewInit() failed!" << endl;
		system("pause");
		return 1;
	}

	genlist();

	if (fullscreen)
	{
		//glutFullScreen();
	}
	glutMainLoop();
	return 0;
}

void loadobj(char* filename)
{
	fstream config(filename, ios::in);
	if (!config)
	{
		printf("Config file not exist!\n");
		system("pause");
		exit(4);
	}

	string command;
	string obj;
	string tga;
	double x = 0, y = 0, z = 0;
	double sx = 1, sy = 1, sz = 1;

	while (true)
	{
		config >> command;
		if (config.eof()) break;
		if (strcmpi(command.c_str(), "OBJECT") == 0)
		{
			objn++;
			obj = "";
			tga = "";
			x = 0; y = 0; z = 0;
			sx = 1; sy = 1; sz = 1;
			while (true)
			{
				config >> command;
				if (config.eof()) break;
				if (strcmpi(command.c_str(), "OBJ") == 0)
				{
					if (obj != "")
					{
						printf("Redundant OBJ tag in %s\n", filename);
						system("pause");
						exit(8);
					}
					config >> obj;
					if (config.eof()) break;
				}
				else if (strcmpi(command.c_str(), "TGA") == 0)
				{
					config >> tga;
					if (config.eof()) break;
				}
				else if (strcmpi(command.c_str(), "POS") == 0)
				{
					config >> x >> y >> z;
					if (config.eof()) break;
				}
				else if (strcmpi(command.c_str(), "OBJEND") == 0)
				{
					break;
				}
				else if (strcmpi(command.c_str(), "SCALE") == 0)
				{
					config >> sx >> sy >> sz;
					if (config.eof()) break;
				}
				else
				{
					printf("Unrecognized tag: %s\n", command.c_str());
					system("pause");
					exit(5);
				}
			}

			if (obj == "")
			{
				continue;
			}

			ljxObject newobj;
			newobj.x = x; newobj.y = y; newobj.z = z;
			newobj.obj = obj;
			newobj.tga = tga;
			newobj.sx = sx; newobj.sy = sy; newobj.sz = sz;
			newobj.osx = sx; newobj.osy = sy; newobj.osz = sz;
			GLMmodel *boat;
			boat = glmReadOBJ(obj.c_str());
			if (!boat)
			{
				printf("Failed to load obj!\n");
				system("pause");
				exit(2);
			}
			else
			{
				newobj.model = *boat;
				printf("Obj read succeeded for %s\n", obj.c_str());
			}
			if (tga != "")
			{
				newobj.havetext = true;
				if (!LoadTGA(&newobj.texture, tga.c_str()))
				{
					printf("Failed to load TGA file\n");
					system("pause");
					exit(6);
				}
				else
				{
					printf("TGA read succeeded for %s\n", tga.c_str());
				}
			}
			glmUnitize(&newobj.model);
			glmFacetNormals(&newobj.model);
			glmVertexNormals(&newobj.model, 90.0);
			objs.push_back(newobj);
			printf("Newobject pushed\n");
		}
		else
		{
			printf("Unrecognized tag: %s\n", command.c_str());
			system("pause");
			exit(5);
		}
	}
}

void display()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();									// Reset The Current Modelview Matrix
	GLfloat lookat[3];
	generateEye(eye, lookat);
	gluLookAt(eye[0], eye[1], eye[2],
						lookat[0], lookat[1], lookat[2],
						0, 1, 0);
	//cout << eye[0] << " $$$ " << eye[1] << " %%% " << eye[2] <<endl;
	//cout << viewangle[0] << " ))) " << viewangle[1] << endl;
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glEnable(GL_DEPTH_TEST);

	light();
	draw(GL_RENDER);
	drawCross();
	getFPS();
	glutSwapBuffers();
	
	Sleep(1000 / 60);
}

void reshape(int width, int height)
{
	if (height == 0)
	{
		height = 1;
	}

	gheight = height;
	gwidth = width;
	printf("heigth=%d\n", height);
	printf("width=%d\n", width);
	updateView(gwidth, gheight);
}

void keyboard(unsigned char key, int x, int y)
{
	double const dc = 0.01;
	const double step = 0.05;
	const double dangle = M_PI / 24;
	switch (key)
	{
		case 27:
			exit(0);
		case 'a':
								//left
			moveeye(LEFT, step);
								break;
		case 'd': 
								//right		
			moveeye(RIGHT, step);
								break;
		case 'w': 
								//foreward
			moveeye(FOREWARD, step);
								break;
		case 's': 
								//backward
			moveeye(BACKWARD, step);
								break;
		case 'z': 
								//up
			moveeye(UP, step);
								break;
		case 'c':
			          //down
			moveeye(DOWN, step);
								break;
		case 'v':
			if (chosen != -1)
			{
				ljxObject newobj;
				newobj = objs[chosen];
				GLfloat pos[3];
				generateEye(eye, pos);
				newobj.x = pos[0];
				newobj.y = pos[1];
				newobj.z = pos[2];
				objs[chosen].chosen = false;
				newobj.chosen = false;
				chosen = -1;
				objs.push_back(newobj);
				genlist();
			}
			break;
		case 'b':
			if (chosen != -1)
			{
				ObjectList::iterator i = objs.begin() + chosen;
				objs.erase(i);
				genlist();
				chosen = -1;
			}
			break;
		case 'r':
			eye[0] = 0;
			eye[1] = 0;
			eye[2] = 8;
			viewangle[0] = -M_PI / 2;
			viewangle[1] = M_PI / 2;
			break;
		case 'p':
			model = !model;
			updateView(gwidth, gheight);
			break;
		case ' ':
			ro = !ro;
			break;
		case 'e':
			light0pos[0] = eye[0];
			light0pos[1] = eye[1];
			light0pos[2] = eye[2];
			break;
		case 'x':
			if (chosen != -1)
			{
				GLfloat pos[3];
				generateEye(eye, pos);
				objs[chosen].x = pos[0];
				objs[chosen].y = pos[1];
				objs[chosen].z = pos[2];
			}
			break;
		case '1':
			light0col[0] += dc;
			if (light0col[0] > 1.0) light0col[0] -= 1.0;
			break;
		case '2':
			light0col[1] += dc;
			if (light0col[1] > 1.0) light0col[1] -= 1.0;
			break;
		case '3':
			light0col[2] += dc;
			if (light0col[2] > 1.0) light0col[2] -= 1.0;
			break;
		case 't':
			withtext = !withtext;
			break;
		case 'q':
			showlightpos = !showlightpos;
			break;
		case '[':
			if (chosen != -1)
			{
				if (objs[chosen].sx > 0) objs[chosen].sx -= 0.05;
				if (objs[chosen].sy > 0) objs[chosen].sy -= 0.05;
				if (objs[chosen].sz > 0) objs[chosen].sz -= 0.05;
			}
			break;
		case ']':
			if (chosen != -1)
			{
				objs[chosen].sx += 0.05;
				objs[chosen].sy += 0.05;
				objs[chosen].sz += 0.05;
			}
			break;
		case 'o':
			if (chosen != -1)
			{
				objs[chosen].sx = objs[chosen].osx;
				objs[chosen].sy = objs[chosen].osy;
				objs[chosen].sz = objs[chosen].osz;
			}
			break;
		case 'f':
			fullscreen = !fullscreen;
			if (fullscreen)
			{
				glutFullScreen();
			}
			else
			{
			}
			break;
	}
}

void mouse(GLint button, GLint action, GLint x, GLint y)
{
	cout << "button: " << button << endl;
	if (button == 3)
	{
		wheel(button, 120, x, y);
	}
	if (button == 4)
	{
		wheel(button, -120, x, y);
	}
	if (action == GLUT_DOWN){
		if (button == GLUT_RIGHT_BUTTON) mouserdown = GL_TRUE;
		if (button == GLUT_LEFT_BUTTON) mouseldown = GL_TRUE;
		if (button == GLUT_MIDDLE_BUTTON) mousemdown = GL_TRUE;
	}
	else {
		if (button == GLUT_RIGHT_BUTTON) mouserdown = GL_FALSE;
		if (button == GLUT_LEFT_BUTTON){ mouseldown = GL_FALSE; glutSetCursor(GLUT_CURSOR_RIGHT_ARROW); }
		if (button == GLUT_MIDDLE_BUTTON) mousemdown = GL_FALSE;
	}
	mousex = x;
	mousey = y;
	GLint n, vp[4], get;
	if (button == GLUT_LEFT_BUTTON && action == GLUT_DOWN)
	{
		glutSetCursor(GLUT_CURSOR_NONE);
		glGetIntegerv(GL_VIEWPORT, vp);
		glSelectBuffer(PBSIZE, pb);
		glRenderMode(GL_SELECT);
		glInitNames();
		glMatrixMode(GL_PROJECTION);

		glPushMatrix();
		glLoadIdentity();
		double whRatio = (GLfloat)gwidth / (GLfloat)gheight;
		if (model)
		{
			gluPerspective(45.0f, whRatio, 0.1f, 100.0f);
		}
		else
		{
			glOrtho(-3, 3, -3, 3, -100, 100);
		}
		gluPickMatrix(GLdouble(x), GLdouble(vp[3] - y), 3, 3, vp);
		glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
		draw(GL_SELECT);
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();

		glMatrixMode(GL_MODELVIEW);
		n = glRenderMode(GL_RENDER);
		updateView(gwidth, gheight);
		GLfloat lookat[3];
		generateEye(eye, lookat);
		gluLookAt(eye[0], eye[1], eye[2],
							lookat[0], lookat[1], lookat[2],
							0, 1, 0);
		cout << n << " hits : " << endl;

		get = processpick(n, pb);
		if (get != 0)
		{
			ObjectList::iterator j;
			bool state = objs[get - 1].chosen;
			for (j = objs.begin();
					 j != objs.end(); j++)
			{
				j->chosen = false;
			}
			objs[get - 1].chosen = !state;
			chosen = get - 1;
		}
		else
		{
			ObjectList::iterator j;
			for (j = objs.begin();
					 j != objs.end(); j++)
			{
				j->chosen = false;
			}
			chosen = -1;
		}
	}
	glutPostRedisplay();
}

void motion(int x, int y)
{
	GLint xx, yy;
	xx = x - mousex;
	yy = y - mousey;
	//cout << "xx: " << xx << ", yy" << yy << endl;
	if (mouseldown == GL_TRUE){
		const double da = M_PI / 2;
		//glutSetCursor(GLUT_CURSOR_NONE);
		INPUT input;
		ZeroMemory(&input, sizeof(input));
		input.type = INPUT_MOUSE;
		input.mi.dx = 65535 / 2;
		input.mi.dy = 65535 / 2;
		input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
		//SendInput(1, &input, sizeof(input));
		double da0 = da * (1.0 * xx / GetSystemMetrics(SM_CXSCREEN));
		double da1 = da * (1.0 * yy / GetSystemMetrics(SM_CYSCREEN));
		viewangle[0] += da0;
		if (viewangle[1] + da1 >= 0.0&&viewangle[1] + da1 <= M_PI)
		{
			viewangle[1] += da1;
		}

		while (viewangle[0] > M_PI)
		{
			viewangle[0] -= 2 * M_PI;
		}
		while (viewangle[0] < -M_PI)
		{
			viewangle[0] += 2 * M_PI;
		}
	}
	if (mouserdown == GL_TRUE){
		eye[0] += xx * 0.01 * sin(viewangle[0]);
		eye[2] -= xx * 0.01 * cos(viewangle[0]);
		eye[1] += yy * 0.01 * sin(viewangle[1]);
	}
	if (mousemdown == GL_TRUE){
		eye[0] -= xx * 0.15 * sin(viewangle[1]) * cos(viewangle[0]);
		eye[1] -= xx * 0.15 * cos(viewangle[1]);
		eye[2] -= xx * 0.15 * sin(viewangle[1]) * sin(viewangle[0]);
	}
	mousex = x;
	mousey = y;
	glutPostRedisplay();
}

void idle()
{
	glutPostRedisplay();
}

void generateEye(GLfloat eye[], GLfloat lookat[3])
{
	GLfloat delta[3];
	delta[0] = 1.0 * sin(viewangle[1]) * cos(viewangle[0]);
	delta[1] = 1.0 * cos(viewangle[1]);
	delta[2] = 1.0 * sin(viewangle[1]) * sin(viewangle[0]);
	lookat[0] = eye[0] + delta[0];
	lookat[1] = eye[1] + delta[1];
	lookat[2] = eye[2] + delta[2];
}

void generateEye(GLfloat eye[], GLfloat lookat[3], double step)
{
	GLfloat delta[3];
	delta[0] = step * sin(viewangle[1]) * cos(viewangle[0]);
	delta[1] = step * cos(viewangle[1]);
	delta[2] = step * sin(viewangle[1]) * sin(viewangle[0]);
	lookat[0] = eye[0] + delta[0];
	lookat[1] = eye[1] + delta[1];
	lookat[2] = eye[2] + delta[2];
}

void updateView(int width, int height)
{
	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	float whRatio = (GLfloat)width / (GLfloat)height;

	if (model)
	{
		gluPerspective(45.0, (GLfloat)width / (GLfloat)height, 0.01f, 100.0f);
	}
	else
		glOrtho(-3, 3, -3, 3, -100, 100);

	glMatrixMode(GL_MODELVIEW);
}

void draw(GLenum mode)
{
	glPushMatrix();

	int count = 0;
	for (ObjectList::iterator i = objs.begin();
			 i != objs.end(); i++)
	{
		glPushMatrix();
	  glTranslatef(i->x, i->y, i->z);
		glRotatef(i->angle, 0, 1, 0);
		if (ro)
		{
			if (i->chosen)
			{
				i->angle += dan;
				if (i->angle > 360)
				{
					i->angle -= 360;
				}
			}
		}
		if (mode == GL_SELECT) glPushName(count + 1);
		glScalef(i->sx, i->sy, i->sz);
		if (i->havetext && withtext)
		{
			glCallList(i->lidwithtext);
		}
		else
		{
			glCallList(i->lidwithouttext);
		}
		if (mode == GL_SELECT) glPopName();
		if (i->chosen)
		{
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_LIGHTING);
			glColor3f(1.0, 1.0, 1.0);
			glutWireCube(1.0);
			glEnable(GL_LIGHTING);
		}
		glPopMatrix();
		count++;
	}
	glPopMatrix();
}

void drawCross()
{
	GLfloat lookat[3];
	generateEye(eye, lookat, 0.1);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glPushMatrix();
	glTranslatef(lookat[0], lookat[1], lookat[2]);
	glRotatef(-viewangle[0] / M_PI * 180, 0, 1, 0);
	glRotatef(-viewangle[1] / M_PI * 180, 0, 0, 1);
	glDisable(GL_TEXTURE_2D);
	glutWireCube(0.001);
	glPopMatrix();
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
}

void light()
{
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_POSITION, light0pos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0col);
	glEnable(GL_LIGHT0);
	if (showlightpos)
	{
		glPushMatrix();
		glTranslatef(light0pos[0], light0pos[1], light0pos[2]);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glColor3f(1.0, 1.0, 1.0);
		glutSolidCube(0.01);
		glEnable(GL_LIGHTING);
		glPopMatrix();
	}
}

void debug_print_eye()
{
	printf("Now eye at (%5.5f, %5.5f, %5.5f)\n", eye[0], eye[1], eye[2]);
}

GLuint processpick(GLint n, GLuint pb[])
{
	GLint i, minn = 0;
	GLuint name, *ptr;
	GLfloat minz = 9999.0;
	ptr = pb;
	for (i = 0; i < n; i++)
	{
		name = *ptr;
		cout << "This object has " << name << " names, include " << endl;
		ptr++;
		cout << ((GLfloat)*ptr / 0xFFFFFFFF) << " now min: " << minz << " @ " << minn << endl;
		if (((GLfloat)*ptr / 0xFFFFFFFF) < minz)
		{
			minz = ((GLfloat)*ptr / 0xFFFFFFFF);
			minn = *(ptr + 2);
		}
		ptr += 2;
		for (int j = 0; j < name; j++)
		{
			cout << j << " ) " << *ptr << endl;
			ptr++;
		}
	}
	cout << "last min: " << minz << " @ " << minn << endl;
	return minn;
}

void genlist()
{
	for (ObjectList::iterator i = objs.begin();
			 i != objs.end();
			 i++)
	{
		i->lidwithouttext = glGenLists(1);
		glNewList(i->lidwithouttext, GL_COMPILE);
		glDisable(GL_TEXTURE_2D | GLM_MATERIAL);
		glmDraw(&i->model, GLM_SMOOTH);
		glEndList();

		i->lidwithtext = glGenLists(1);
		glNewList(i->lidwithtext, GL_COMPILE);
		glmDraw(&i->model, &i->texture,
						GLM_SMOOTH | GLM_MATERIAL | GLM_TEXTURE);
		glEndList();
	}
}

void getFPS()
{
	static int frame = 0, time, timebase = 0;
	static char buffer[256];

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	frame++;
	time = glutGet(GLUT_ELAPSED_TIME);
	if (time - timebase > 1000)
	{
		char choosing[255];
		ZeroMemory(choosing, sizeof(choosing));
		if(chosen!=-1) sprintf(choosing, "Choosing Item %d Scale:(%2.2lf, %2.2lf, %2.2lf)",
													 chosen, objs[chosen].sx, objs[chosen].sy, objs[chosen].sz);
		sprintf(buffer, "FPS:%4.2f Light Color in RGB (%4.2f, %4.2f, %4.2f) %s %s %s %s",
						frame*1000.0 / (time - timebase), light0col[0], light0col[1], light0col[2],
						(ro) ? "Rotating " : "Static ", (withtext)?"Texture Rendering":"Texture Ignored",
						(chosen == -1) ? "" : choosing,
						(chosen == -1)? "" : objs[chosen].obj.c_str());
		timebase = time;
		frame = 0;
	}

	char *c;
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);  // 选择投影矩阵
	glPushMatrix();               // 保存原矩阵
	glLoadIdentity();             // 装入单位矩阵
	glOrtho(0, 480, 0, 480, 1, -1);    // 位置正投影
	glMatrixMode(GL_MODELVIEW);   // 选择Modelview矩阵
	glPushMatrix();               // 保存原矩阵
	glLoadIdentity();             // 装入单位矩阵

	glRasterPos2f(10, 10);
	glColor3f(1.0, 1.0, 1.0);
	for (c = buffer; *c != '\0'; c++)
	{
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
	}
	glMatrixMode(GL_PROJECTION);  // 选择投影矩阵
	glPopMatrix();                // 重置为原保存矩阵
	glMatrixMode(GL_MODELVIEW);   // 选择Modelview矩阵
	glPopMatrix();                // 重置为原保存矩阵
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
}

void wheel(int button, int dir, int x, int y)
{
	double step;
	const double ratio = 0.05;
	step = ratio*dir;
	moveeye(FOREWARD, step);
}

void moveeye(Direction dir, double step)
{
	switch (dir)
	{
		case UP:
			eye[1] += step * sin(viewangle[1]);
			break;
		case DOWN:
			eye[1] -= step * sin(viewangle[1]);
			break;
		case LEFT:
			eye[0] += step * sin(viewangle[0]);
			eye[2] -= step * cos(viewangle[0]);
			break;
		case RIGHT:
			eye[0] -= step * sin(viewangle[0]);
			eye[2] += step * cos(viewangle[0]);
			break;
		case FOREWARD:
			eye[0] += step * sin(viewangle[1]) * cos(viewangle[0]);
			eye[1] += step * cos(viewangle[1]);
			eye[2] += step * sin(viewangle[1]) * sin(viewangle[0]);
			break;
		case BACKWARD:
			eye[0] -= step * sin(viewangle[1]) * cos(viewangle[0]);
			eye[1] -= step * cos(viewangle[1]);
			eye[2] -= step * sin(viewangle[1]) * sin(viewangle[0]);
			break;
	}
}