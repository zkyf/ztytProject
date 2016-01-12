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
typedef GLfloat GLpoint3f[3];

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
void passivemotion(int x, int y);
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
void getFPS();
void processColor(GLMmodel *object, double sx);

const double dan = 1;

ObjectList objs;
int gwidth, gheight;
bool model = true;
bool ro = false;
bool follow = true;
bool withtext = true;
bool withlimit = false;
bool showlightpos = false;
bool fullscreen = true;
bool mode = false;
bool withscene = false;
int objn = 0;
int chosen = -1;
double step = 0.1;
double nx, px, ny, py, nz, pz; // limit of the position of the camera
GLint lid;
HWND hWnd;
HINSTANCE hInstance;
GLfloat eye[3] = { 0.0, 0.0, 0 };
GLfloat viewangle[2] = { -M_PI / 2, M_PI / 2 };
GLfloat light0col[3] = { 1.0, 1.0, 1.0 };
GLfloat light0pos[4] = { 0.0, 0.0, 0.0, 1.0 };
GLuint pb[PBSIZE];
GLpoint3f userlightlist[10];
GLuint userlighttype[10];
int userlightcount = 0;

int main(int argc, char *argv[])
{
	//system("gesture.exe");

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(640, 480);
	glutCreateWindow("专题研讨");

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
	glutPassiveMotionFunc(passivemotion);
	glutIdleFunc(idle);
	


	if (glewInit() != GLEW_OK)
	{
		cout << "glewInit() failed!" << endl;
		system("pause");
		return 1;
	}

	genlist();

	if (fullscreen)
	{
		glutFullScreen();
	}
	glutMainLoop();
	return 0;
}

void loadobj(char* filename)
{
	fstream config(filename, ios::in);
	if (!config)
	{
		config.open("scene.config", ios::in);
		if (config.fail())
		{
			printf("Config file not exist!\n");
			system("pause");
			exit(4);
		}
	}

	int max;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);
	cout << "The max number of textures:　" << max << endl;

	string command;
	string obj;
	string tga;
	double x = 0, y = 0, z = 0;
	double sx = 1, sy = 1, sz = 1;
	double ax = 0, ay = 0, az = 0;
	double rotate = 0;
	bool nonnormal = false;
	bool getscene = false;
	bool lineartext = false;
	while (true)
	{
		config >> command;
		if (command[0] == '#')
		{
			getline(config, command);
			continue;
		}
		if (config.eof()) break;
		if (strcmpi(command.c_str(), "OBJECT") == 0)
		{
			objn++;
			nonnormal = false;
			getscene = false;
			lineartext = false;
			obj = "";
			tga = "";
			x = 0; y = 0; z = 0;
			sx = 1; sy = 1; sz = 1;
			ax = 0; ay = 0; az = 0;
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
				}
				else if (strcmpi(command.c_str(), "NONNORMAL") == 0)
				{
					nonnormal == true;
				}
				else if (strcmpi(command.c_str(), "SCENE") == 0)
				{
					if (withscene)
					{
						printf("Redundant scene specified!\n");
						break;
					}
					withscene = true;
					getscene = true;
				}
				else if (strcmpi(command.c_str(), "LINEARTEXT") == 0)
				{
					lineartext = true;
				}
				else if (strcmpi(command.c_str(), "ROTATE") == 0)
				{
					printf("Rotate get\n");
					config >> rotate >> ax >> ay >> az;
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
			newobj.ax = ax; newobj.ay = ay; newobj.az = az;
			newobj.rotate = rotate;
			newobj.selectable = !getscene;
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
			glmUnitize(boat);
			if (getscene)
			{
				printf("Scene detected\n");
				glmGetBox(boat, nx, px, ny, py, nz, pz);
				nx *= sx * 0.9; ny *= sy * 0.9; nz *= sz * 0.9;
				px *= sx * 0.9; py *= sy * 0.9; pz *= sz * 0.9;
			}
			if (!nonnormal)
			{
				glmFacetNormals(boat);
				glmVertexNormals(boat, 90.0);
			}

			if(lineartext) glmLinearTexture(boat);
			processColor(boat, sx);
			newobj.lidwithouttext = glmList(boat, GLM_MATERIAL | GLM_SMOOTH);
			newobj.lidwithtext = glmList(boat, GLM_SMOOTH | GLM_MATERIAL | GLM_TEXTURE);
			if (newobj.lidwithouttext == 0 || newobj.lidwithtext == 0)
			{
				printf("Failed to create display list for %s\n", obj.c_str());
			}

			objs.push_back(newobj);
			printf("Newobject pushed\n");
			glmDelete(boat);
		}
		else if (strcmpi(command.c_str(), "STEP") == 0)
		{
			if (config.eof()) break;
			config >> step;
			printf("step changed to %5.5lf\n", step);
			if (config.eof()) break;
		}
		else if (strcmpi(command.c_str(), "LIGHT") == 0)
		{
			GLpoint3f newlight;
			string type;
			if (config.eof()) break;
			config >> newlight[0] >> newlight[1] >> newlight[2] >> type;
			if (userlightcount >= 8)
			{
				printf("Warning: the scene can only include at most 8 lights. This light will be ignored\n");
				continue;
			}
			userlighttype[userlightcount] = GL_AMBIENT;
			if (strcmpi(type.c_str(), "A") == 0) userlighttype[userlightcount] = GL_AMBIENT;
			else if (strcmpi(type.c_str(), "D") == 0) userlighttype[userlightcount] = GL_DIFFUSE;
			else if (strcmpi(type.c_str(), "AD") == 0) userlighttype[userlightcount] = GL_AMBIENT_AND_DIFFUSE;
			else printf("Unknown light type %s\n", type.c_str());
			userlightlist[userlightcount][0] = newlight[0];
			userlightlist[userlightcount][1] = newlight[1];
			userlightlist[userlightcount][2] = newlight[2];
			printf("Light No. %d specified at (%5.5lf, %5.5lf, %5.5lf)\n",
						 userlightcount, userlightlist[userlightcount][0], userlightlist[userlightcount][1],
						 userlightlist[userlightcount][2]);
			userlightcount++;
		}
		else if (strcmpi(command.c_str(), "LIGHTCOL") == 0)
		{
			config >> light0col[0] >> light0col[1] >> light0col[2];
			printf("Light color set to (%5.5lf, %5.5lf, %5.5lf)\n", light0col[0], light0col[1], light0col[2]);
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


	draw(GL_RENDER);

	drawCross();
	getFPS();
	light();
	//drawscene();
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
				newobj.x = eye[0];
				newobj.y = eye[1];
				newobj.z = eye[2];
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
			eye[2] = 0;
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
				objs[chosen].x = eye[0];
				objs[chosen].y = eye[1];
				objs[chosen].z = eye[2];
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
				if (objs[chosen].sx > 0) objs[chosen].sx -= step;
				if (objs[chosen].sy > 0) objs[chosen].sy -= step;
				if (objs[chosen].sz > 0) objs[chosen].sz -= step;
			}
			break;
		case ']':
			if (chosen != -1)
			{
				objs[chosen].sx += step;
				objs[chosen].sy += step;
				objs[chosen].sz += step;
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
		case 'y':
			mode = !mode;
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

		gluPickMatrix(GLdouble(x), GLdouble(vp[3] - y), 0.5, 0.5, vp);
		double whRatio = (GLfloat)gwidth / (GLfloat)gheight;
		if (model)
		{
			gluPerspective(45.0f, whRatio, 0.1f, 100.0f);
		}
		else
		{
			glOrtho(nx, px, ny, py, nz, pz);
		}
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
			
			if (chosen == -1)
			{
				chosen = get - 1;
				objs[get - 1].chosen = true;
			}
			else chosen = -1;
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
	if (mode) return;
	GLint xx, yy;
	xx = x - mousex; xx = -xx;
	yy = y - mousey; yy = -yy;
	//cout << "xx: " << xx << ", yy" << yy << endl;
	if (mouseldown == GL_TRUE){
		const double da = M_PI / 2;
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
	if (mouserdown == GL_TRUE)
	{
		eye[0] += xx * step * 0.1 * sin(viewangle[0]);
		eye[2] -= xx * step * 0.1 * cos(viewangle[0]);
		eye[1] += yy * step * 0.1 * sin(viewangle[1]);
		if (withscene)
		{
			if (eye[0] <= nx || eye[0] >= px ||
					eye[1] <= ny || eye[1] >= py ||
					eye[2] <= nz || eye[2] >= pz)
			{
				eye[0] -= xx * step * 0.1 * sin(viewangle[0]);
				eye[2] += xx * step * 0.1 * cos(viewangle[0]);
				eye[1] -= yy * step * 0.1 * sin(viewangle[1]);
			}
		}
	}
	if (mousemdown == GL_TRUE)
	{
		eye[0] -= xx * step * 0.1 * sin(viewangle[1]) * cos(viewangle[0]);
		eye[1] -= xx * step * 0.1 * cos(viewangle[1]);
		eye[2] -= xx * step * 0.1 * sin(viewangle[1]) * sin(viewangle[0]);
		if (withscene)
		{
			if (eye[0] <= nx || eye[0] >= px ||
					eye[1] <= ny || eye[1] >= py ||
					eye[2] <= nz || eye[2] >= pz)
			{
				eye[0] += xx * step * 0.1 * sin(viewangle[1]) * cos(viewangle[0]);
				eye[1] += xx * step * 0.1 * cos(viewangle[1]);
				eye[2] += xx * step * 0.1 * sin(viewangle[1]) * sin(viewangle[0]);
			}
		}
	}
	mousex = x;
	mousey = y;
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
		glOrtho(nx, px, ny, py, nz, pz);

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
		glRotatef(i->rotate, i->ax, i->ay, i->az);
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
		glPushMatrix();
		if (withtext)
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
	if (userlightcount == 0)
	{
		glLightfv(GL_LIGHT0, GL_POSITION, light0pos);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light0col);
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
	else
	{
		for (int i = 0; i < userlightcount; i++)
		{
			glLightfv(GL_LIGHT0 + i, GL_POSITION, userlightlist[i]);
			glLightfv(GL_LIGHT0 + i, userlighttype[i], light0col);
			glEnable(GL_LIGHT0 + i);
			if (showlightpos)
			{
				glPushMatrix();
				glTranslatef(userlightlist[i][0], userlightlist[i][1], userlightlist[i][2]);
				glDisable(GL_TEXTURE_2D);
				glDisable(GL_LIGHTING);
				glColor3f(1.0, 1.0, 1.0);
				glutSolidCube(0.01);
				glEnable(GL_LIGHTING);
				glPopMatrix();
			}
		}
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
	GLuint now;
	ptr = pb;
	for (i = 0; i < n; i++)
	{
		name = *ptr;
		cout << "This object has " << name << " names, include " << endl;
		ptr++;
		cout << ((GLfloat)*ptr / 0xFFFFFFFF) << " now min: " << minz << " @ " << minn << endl;
		if (((GLfloat)*ptr / 0xFFFFFFFF) < minz)
		{
			now = *(ptr + 2) - 1;
			if (objs[now].selectable)
			{
				minz = ((GLfloat)*ptr / 0xFFFFFFFF);
				minn = now + 1;
			}
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
	//glPushMatrix();
	//glLoadIdentity();
	//for (ObjectList::iterator i = objs.begin();
	//		 i != objs.end();
	//		 i++)
	//{
	//	i->lidwithouttext = glGenLists(1);
	//	glNewList(i->lidwithouttext, GL_COMPILE);
	//	glDisable(GL_TEXTURE_2D);
	//	glmDraw(&i->model, GLM_SMOOTH | GLM_MATERIAL);
	//	glEndList();

	//	i->lidwithtext = glGenLists(1);
	//	glNewList(i->lidwithtext, GL_COMPILE);
	//	glmDraw(&i->model,
	//					GLM_SMOOTH | GLM_MATERIAL | GLM_TEXTURE);
	//	glEndList();
	//}
	//glPopMatrix();
}

void getFPS()
{
	static int frame = 0, time, timebase = 0;
	static char buffer[256], buf2[256];

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	frame++;
	time = glutGet(GLUT_ELAPSED_TIME);
	if (time - timebase > 1000)
	{
		char choosing[255];
		ZeroMemory(choosing, sizeof(choosing));
		if(chosen!=-1) sprintf(choosing, "Item %d Scale:(%2.2lf, %2.2lf, %2.2lf) @ (%2.2lf, %2.2lf, %2.2lf)",
													 chosen, objs[chosen].sx, objs[chosen].sy, objs[chosen].sz,
													 objs[chosen].x, objs[chosen].y, objs[chosen].z);
		sprintf(buffer, "Light Color (%4.2f, %4.2f, %4.2f) %s %s %s %s",
						light0col[0], light0col[1], light0col[2],
						(ro) ? "Rotating " : "Static ", (withtext)?"Texture":"No Texture",
						(chosen == -1) ? "" : choosing,
						(chosen == -1)? "" : objs[chosen].obj.c_str());
		sprintf(buf2, "FPS:%4.2f Pos: (%5.5lf, %5.5lf, %5.5lf)", frame*1000.0 / (time - timebase), eye[0], eye[1], eye[2]);
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

	glRasterPos2f(10, 460);
	for (c = buf2; *c != '\0'; c++)
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
	if (withscene)
	{
		if (eye[0] <= nx || eye[0] >= px ||
				eye[1] <= ny || eye[1] >= py ||
				eye[2] <= nz || eye[2] >= pz)
		{
			moveeye(dir, -step);
		}
	}
}

void passivemotion(int x, int y)
{
	if (!mode)
	{
		glutSetCursor(GLUT_CURSOR_CROSSHAIR);
		return;
	}
	const double da = M_PI / 2;
	glutSetCursor(GLUT_CURSOR_NONE);
	INPUT input;
	ZeroMemory(&input, sizeof(input));
	input.type = INPUT_MOUSE;
	input.mi.dx = 65535 / 2;
	input.mi.dy = 65535 / 2;
	input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
	SendInput(1, &input, sizeof(input));
	double da0 = da * (1.0 * x / GetSystemMetrics(SM_CXSCREEN) - 0.5);
	double da1 = da * (1.0 * y / GetSystemMetrics(SM_CYSCREEN) - 0.5);
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
	mousex = x;
	mousey = y;
}

double fx(double x)
{
	return (x - 1.0)*0.7 + 1.0;
}

void processColor(GLMmodel *object, double sx)
{
	if (!object->materials)
	{
		return;
	}
	for (int i = 0; i < object->nummaterials; i++)
	{
		object->materials[i].ambient[0] *= fx(sx);
		object->materials[i].ambient[1] *= fx(sx);
		object->materials[i].ambient[2] *= fx(sx);
		object->materials[i].ambient[3] *= fx(sx);

		object->materials[i].diffuse[0] *= fx(sx);
		object->materials[i].diffuse[1] *= fx(sx);
		object->materials[i].diffuse[2] *= fx(sx);
		object->materials[i].diffuse[3] *= fx(sx);
	}
}