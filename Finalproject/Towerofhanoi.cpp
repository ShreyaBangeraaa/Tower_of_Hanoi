#include <stdlib.h>
#include <stdio.h>
#include <glut.h>

#define HEIGHT 0.2f
#define ROD 0.03f
#define SLICES 10
#define INNERSLICES 5
#define LOOPS 1
#define FPS 100 /* more looks nicer, uses more cpu power */
#define FEM 1000.0 / FPS
#define FSEM 0.0005f /* speed (bigger is faster) frame speed each millisecond*/

struct config
{
	GLfloat gap;
	GLfloat pinradius;
	GLfloat pinheight;
};

struct action
{
	char fromstack;
	char tostack;
	struct action *next;
};
typedef struct action action;

struct actions
{
	action *head;
	action *tail;
};
typedef struct actions actions;
struct disk
{
	char color;
	GLfloat radius;
	struct disk *next;
	struct disk *prev;
};
typedef struct disk disk;

struct stack
{
	disk *bottom;
	disk *top;
};
typedef struct stack stack;
void *current = GLUT_BITMAP_TIMES_ROMAN_24;
int disks = 3, x = 0, z; //initial number of disks
GLfloat rotX, rotY, zoom, offsetY = 1.5, speed;
GLUquadricObj *quadric;
GLfloat pos;
GLboolean fullscreen;
stack pin[3];
float pinheight[3];
struct config config;
actions actqueue;
action *curaction;
disk *curdisk;
int duration;

int draw, maxdraws;
//function prototypes
void moveDisk(int param);
void hanoiinit(void);
void reset();
void Display(void);
void hanoi(actions *queue, const int n, const char pin1, const char pin2, const char pin3);
void push(stack *pin, disk *item);
disk *pop(stack *pin);
void drawDisk(GLUquadricObj **quadric, const GLfloat outer, const GLfloat inner);
void drawPin(GLUquadricObj **quadric, const GLfloat radius, const GLfloat height);
void drawAllPins(GLUquadricObj **quadric, const GLfloat radius, const GLfloat height, const GLfloat gap);

//tower of hanoi recursive algorithm
void hanoi(actions *queue, const int n, const char pin1, const char pin2, const char pin3)
{
	action *curaction;
	if (n > 0)
	{
		hanoi(queue, n - 1, pin1, pin3, pin2);
		// push action into action queue
		curaction = (action *)malloc(sizeof(action));
		curaction->next = NULL;
		curaction->fromstack = pin1;
		curaction->tostack = pin3;
		if (queue->head == NULL)
			queue->head = curaction;
		if (queue->tail != NULL)
			queue->tail->next = curaction;
		queue->tail = curaction;
		hanoi(queue, n - 1, pin2, pin1, pin3);
	}
}

/** push item to pin */
void push(stack *pin, disk *item)
{
	item->next = NULL;
	if (pin->bottom == NULL)
	{
		pin->bottom = item;
		pin->top = item;
		item->prev = NULL;
	}
	else
	{
		pin->top->next = item;
		item->prev = pin->top;
		pin->top = item;
	}
}

/** pop item from pin */
disk *pop(stack *pin)
{
	disk *tmp;
	if (pin->top != NULL)
	{
		tmp = pin->top;
		if (pin->top->prev != NULL)
		{
			pin->top->prev->next = NULL;
			pin->top = tmp->prev;
		}
		else
		{
			pin->bottom = NULL;
			pin->top = NULL;
		}
		return tmp;
	}
	return NULL;
}

void drawDisk(GLUquadricObj **quadric, const GLfloat outer, const GLfloat inner)
{
	glPushMatrix();
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	gluCylinder(*quadric, outer, outer, HEIGHT, SLICES, LOOPS);
	gluQuadricOrientation(*quadric, GLU_INSIDE);
	if (inner > 0)
		gluCylinder(*quadric, inner, inner, HEIGHT, INNERSLICES, LOOPS);
	gluDisk(*quadric, inner, outer, SLICES, LOOPS);
	gluQuadricOrientation(*quadric, GLU_OUTSIDE);
	glTranslatef(0.0, 0.0, HEIGHT);
	gluDisk(*quadric, inner, outer, SLICES, LOOPS);
	gluQuadricOrientation(*quadric, GLU_OUTSIDE);
	glPopMatrix();
}

void drawPin(GLUquadricObj **quadric, const GLfloat radius, const GLfloat height)
{
	glPushMatrix();
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	//glRotatef(90.0,0.0,1.0, 1.0);
	gluCylinder(*quadric, radius, radius, HEIGHT / 2, SLICES, LOOPS); //surface between upper and lower part of the base
	gluQuadricOrientation(*quadric, GLU_INSIDE);

	gluDisk(*quadric, 0.0, radius, SLICES, LOOPS); //lower part of base
	gluQuadricOrientation(*quadric, GLU_OUTSIDE);
	glTranslatef(0.0, 0.0, HEIGHT / 2);
	//glColor3f(0,0,1);
	gluDisk(*quadric, 0.0, radius, SLICES, LOOPS);				 //upper part of base
	gluCylinder(*quadric, ROD, ROD, height, INNERSLICES, LOOPS); //draw rod
	glTranslatef(0.0, 0.0, height);
	//glColor3f(0,0,1);
	gluDisk(*quadric, 0.0, ROD, INNERSLICES, LOOPS); //top of rod
	glPopMatrix();
}

void drawAllPins(GLUquadricObj **quadric, const GLfloat radius, const GLfloat height, const GLfloat gap)
{
	glPushMatrix();
	glColor3f(0.6, 0.3, 0.6);
	drawPin(quadric, radius, height); //middle pin
	glTranslatef(-gap, 0.0, 0.0);
	glColor3f(0.3, 0.2, 0.4);
	drawPin(quadric, radius, height); //first pin
	glTranslatef(gap * 2, 0.0, 0.0);
	glColor3f(0.5, 0.6, 1.4);
	drawPin(quadric, radius, height); //last pin
	glPopMatrix();
}

void drawBitmapString(float x, float y, float z, char *string)
{
	char *c;
	glRasterPos3f(x, y, z);
	for (c = string; *c != '\0'; c++)
	{
		glColor3f(0, 0.0, 1.0);
		glutBitmapCharacter(current, *c);
	}
}

void drawBitmapInt(GLfloat x, GLfloat y, GLfloat z, int number)
{
	char string[17];
	sprintf(string, "%d", number);
	drawBitmapString(x, y, z, string);
}

void populatePin(void)
{
	int i;
	disk *cur;
	GLfloat radius = 0.12f * disks;
	for (i = 0; i < disks; i++)
	{
		cur = (disk *)malloc(sizeof(disk));
		cur->color = (char)i % 6;
		cur->radius = radius;
		push(&pin[0], cur);
		radius -= 0.1;
	}
	duration = 0;
	draw = 0;
}

void clearPins(void)
{
	int i;
	disk *cur, *tmp;
	free(curdisk);
	curdisk = NULL;
	for (i = 0; i < 3; i++)
	{
		cur = pin[i].top;
		while (cur != NULL)
		{
			tmp = cur->prev;
			free(cur);
			cur = tmp;
		}
		pin[i].top = NULL;
		pin[i].bottom = NULL;
	}
}

void hanoiinit(void)
{
	GLfloat radius;
	speed = 0.005;						  // FSEM*FEM;
	radius = 0.13f * disks;			  //radius of first disk
	config.pinradius = radius + 0.15f; //radius of base
	config.gap = radius * 2 + 0.65f;	  //radius of 2 first disks +gap between them/distance between 2 centers
	config.pinheight = disks * HEIGHT * 1.8 + 0.3f;
	maxdraws = (2 << (disks - 1)) - 1; //calculate minimum number of moves
	populatePin();
	/* calculate actions; initialize action list */
	actqueue.head = NULL;
	hanoi(&actqueue, disks, 0, 1, 2);
	curaction = actqueue.head;
	curdisk = pop(&pin[(int)curaction->fromstack]);
	pos = 0.001;
}

void reset(void)
{
	clearPins();
	populatePin();
	/* reset actions */
	curaction = actqueue.head;
	curdisk = pop(&pin[(int)curaction->fromstack]);
	pos = 0.001;
	speed = 0;
}

void hanoicleanup(void) //clean up the sequence of operation stored in the structure actqueue,and delete the quadricobject
{
	action *acur, *atmp;
	clearPins();
	acur = actqueue.head;
	do
	{
		atmp = acur->next;
		free(acur);
		acur = atmp;
	} while (acur != NULL);
	gluDeleteQuadric(quadric);
}
void setColor(const int color)
{
	switch (color)
	{
	case 0:
		glColor3f(1.0, 0.0, 0.0);
		break;
	case 1:
		glColor3f(0.0, 1.0, 0.0);
		break;
	case 2:
		glColor3f(1.0, 1.0, 0.0);
		break;
	case 3:
		glColor3f(1.0, 0.0, 1.0);
		break;
	case 4:
		glColor3f(0.9, 0.0, 1.0);
		break;
	case 5:
		glColor3f(0.0, 0.0, 0.0);
		break;
	}
}

void Init(void)
{
	const GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};
	const GLfloat mat_shininess[] = {50.0};
	const GLfloat light_position[] = {0.0, 1.0, 1.0, 0.0};
	glShadeModel(GL_SMOOTH);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);		   /* draw polygons filled */
	glClearColor(1.0, 1.0, 1.0, 1.0);				   /* set screen clear color */
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); /* blending settings */
	glCullFace(GL_BACK);							   /* remove backsides */
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST); //DEPTH BUFFER ALGORITHM
	quadric = gluNewQuadric();
	gluQuadricNormals(quadric, GLU_SMOOTH);
}

/*Is called if the window size is changed */
void Reshape(int width, int height)
{
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, 1, 1.0, 75.0); //
	glMatrixMode(GL_MODELVIEW);
}

/* react to key presses */
void Key(unsigned char key, int x, int y)
{
	switch (key)
	{
	/*
		case '1':
			disks=1;
			hanoiinit();
			reset();
			offsetY=0.9;
			zoom=-1.3;
			gluLookAt(0, 2.2, 3.6+zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0); 
			//reset();
			break;
		case '2':
			disks=2;
			hanoiinit();
			reset();
			offsetY=1.1;
			zoom=-0.8;
			gluLookAt(0.0, 2.2, 3.6+zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0);
			//reset();
			break;
			*/

	case '3':
		disks = 3;
		hanoiinit();
		reset();
		offsetY = 1.6;
		zoom = 1.5;
		gluLookAt(0.0, 2.9, 3.6 + zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0);
		break;
	case 'h':
		disks = 4;
		hanoiinit();
		reset();
		offsetY = 1.6;
		zoom = 1.5;
		gluLookAt(0.0, 2.9, 3.6 + zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0);
		break;
	/*case '5':
		disks = 5;
		hanoiinit();
		reset();
		offsetY = 1.7;
		zoom = 1.4;
		gluLookAt(0.0, 3.2, 3.6 + zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0);
		break;*/
		/*case '6':
			disks=6;
			hanoiinit();
			reset();
			offsetY=1.9;
			zoom=1.8;
			gluLookAt(0.0, 2.2, 3.6+zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0); 
			break;
		case '7':
			disks=7;
			hanoiinit();
			reset();
			offsetY=2.1;
			zoom=2.3;
			gluLookAt(0.0, 2.2, 3.6+zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0);
			break;
		case '8':
			disks=8;
			hanoiinit();
			reset();
			offsetY=2.3;
			zoom=2.8;
			gluLookAt(0.0, 2.2, 3.6+zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0); 
			break;
		case '9':
			disks=9;
			hanoiinit();
			reset();
			offsetY=2.5;
			zoom=3.3;
			gluLookAt(0.0, 2.2, 3.6+zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0); 
			break;*/

	case 's':
		rotX = 0.0;
		rotY = 0.0;
		zoom = 1.5;
		offsetY = 1.6;
		speed = FSEM * FEM;
		break;
	/*case '+':
		zoom -= 0.1;
		break;
	case '-':
		zoom += 0.1;
		break;
	case 'r':
		reset();
		break;
	case 'f':
		if (fullscreen == 0)
		{
			glutFullScreen();
			fullscreen = 1;
		}
		else
		{
			glutReshapeWindow(800, 600);
			glutPositionWindow(50, 50);
			fullscreen = 0;
		}
		break;
	case 's':
		speed += 0.005;
		break;
	case 'x':
		speed -= 0.005;
		if (speed < 0.0)
			speed = 0.0;
		break;
	case 'p':

		speed = 0;
		break;
	case 'a':
		speed = FSEM * FEM;
		break;*/
	case 27:
	case 'q':
		exit(EXIT_SUCCESS);

		break;
	}
	glutPostRedisplay();
}

/*void SpecialKey(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:
		rotX -= 5;
		break;
	case GLUT_KEY_DOWN:
		rotX += 5;
		break;
	case GLUT_KEY_LEFT:
		rotY -= 5;
		break;
	case GLUT_KEY_RIGHT:
		rotY += 5;
		break;
	case GLUT_KEY_PAGE_UP:
		offsetY -= 0.1;
		break;
	case GLUT_KEY_PAGE_DOWN:
		offsetY += 0.1;
		break;
	}
	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		zoom += 0.1;
	}
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
		zoom -= 0.1;
	if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN)
		reset();
}*/


void Display(void)
{
	
	
    glClearColor(0.0, 0.0, 0.0, 1);
	disk *cur;
	int i;
	GLfloat movY;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); /* clear scren */
	glLoadIdentity();
	//drawBitmapString(20.0, 30.0, -60.0, "Moves:");

	//drawBitmapInt(31.0, 30.0, -60.0, draw);
	//drawBitmapString(20.0, 26.0, -60.0, "Minimum Moves:");
	//drawBitmapInt(31.0, 26.0, -60.0, maxdraws);
	
	gluLookAt(0.0, 1.5, 3.6 + zoom, 0.0, offsetY - 0.4, 0.0, 0.0, 1.0, 0.0); // calculate view point
	glRotatef(rotY, 0.0, 1.0, 0.0);											 /* rotate Y axis */
	glRotatef(rotX, 1.0, 0.0, 0.0);											 /* rotate X axis */
	
	drawAllPins(&quadric, config.pinradius, config.pinheight, config.gap); // draw pins
	glTranslatef(-config.gap, HEIGHT / 2, 0.0);							   //load disk to first rod
	glPushMatrix();
	for (i = 0; i < 3; i++)
	{ 
		/* fill pins with disks */
		glPushMatrix();
		pinheight[i] = 0;
		if ((cur = pin[i].bottom) != NULL)
		{
			do
			{
				setColor(cur->color);
				drawDisk(&quadric, cur->radius, ROD);
				glTranslatef(0.0, HEIGHT, 0.0); //draw next disk at height
				pinheight[i] += HEIGHT;
				cur = cur->next;
			} while (cur != NULL);
		}
		glPopMatrix();
		glTranslatef(config.gap, 0.0, 0.0); //disk moves to other rod and stay
	}
	glPopMatrix();
	if (curaction != NULL && curaction->fromstack != -1 && curdisk != NULL)
	{
		if (pos <= 1.0)
		{
			movY = pos * (config.pinheight - pinheight[(int)curaction->fromstack]);
			glTranslatef(config.gap * curaction->fromstack, pinheight[(int)curaction->fromstack] + movY, 0.0); //move up
		}
		else
		{
			if (pos < 2.0 && curaction->fromstack != curaction->tostack)
			{
				if (curaction->fromstack != 1 && curaction->tostack != 1)	 //not middle pin
				{															 /* jump 2 pins */
					glTranslatef(config.gap, config.pinheight + 0.05f, 0.0); //move in x and y direction
					if (curaction->fromstack == 0)
						glRotatef(-(pos - 2.0f) * 180 - 90, 0.0, 0.0, 1.0); //rotate to right in z direction
					else
						glRotatef((pos - 2.0f) * 180 + 90, 0.0, 0.0, 1.0); //rotate to left in z direction
					glTranslatef(0.0, config.gap, 0.0);
				}
				else
				{
					if (curaction->fromstack == 0 && curaction->tostack == 1)
					{
						glTranslatef(config.gap / 2, config.pinheight + 0.05f, 0.0); //move in x and y direction
						glRotatef(-(pos - 2.0f) * 180 - 90, 0.0, 0.0, 1.0);			 //rotate to right in z direction
					}
					else
					{
						if (curaction->fromstack == 2 && curaction->tostack == 1)
						{
							glTranslatef(config.gap / 2 * 3, config.pinheight + 0.05f, 0.0); //move in x and y direction
							glRotatef((pos - 2.0f) * 180 + 90, 0.0, 0.0, 1.0);				 //rotate to left in z direction
						}
						else
						{
							if (curaction->fromstack == 1 && curaction->tostack == 2)
							{
								glTranslatef(config.gap / 2 * 3, config.pinheight + 0.05f, 0.0);
								glRotatef(-(pos - 2.0f) * 180 - 90, 0.0, 0.0, 1.0);
							}
							else
							{
								glTranslatef(config.gap / 2, config.pinheight + 0.05f, 0.0);
								glRotatef((pos - 2.0f) * 180 + 90, 0.0, 0.0, 1.0);
							}
						}
					}
					glTranslatef(0.0, config.gap / 2, 0.0);
				}
				glRotatef(-90, 0.0, 0.0, 1.0);
			}
			else if (pos >= 2.0)
			{																											  /* drop disk down */
				movY = config.pinheight - (pos - 2.0f + speed) * (config.pinheight - pinheight[(int)curaction->tostack]); //mov down in y direction
				glTranslatef(config.gap * curaction->tostack, movY, 0.0);
			}
		}
		setColor(curdisk->color); //draw next disk
		drawDisk(&quadric, curdisk->radius, ROD);
	}
	glutSwapBuffers(); /* swap buffers (double buffering) */
}


void moveDisk(int param)
{
	if (param == 1)
		reset();
	if (curaction != NULL)
	{
		if (pos == 0.0 || pos >= 3.0 - speed)
		{ /* 0--1 -> disk goes upwards, 1--2 "disk in air", 2--3 disk goes downwards*/
			pos = 0.0;
			draw++; //current move
			push(&pin[(int)curaction->tostack], curdisk);
			curaction = curaction->next;
			if (curaction != NULL)
				curdisk = pop(&pin[(int)curaction->fromstack]); //hold the disk on the top in curdisk
		}
		pos += speed; //increase position if increased speed
					  //if(pos > 3.0-FSEM)
					  //	pos = 3.0-FSEM;
		glutTimerFunc((unsigned)FEM, moveDisk, 0);
	}
	else
	{
		curdisk = NULL;
		glutTimerFunc(1000, moveDisk, 1);
	}
	glutPostRedisplay();
}

void timer(int param)
{
	if (curaction != NULL)
	{
		//printf(seconds, "Time: %ds", duration++);
	}
	glutTimerFunc(100, timer, 0);
}

void tower(void)
{

	//glutInit(&argc, argv);

	glutInitWindowPosition(0, 0);
	glutInitWindowSize(1200, 900);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
	z = glutCreateWindow("Towers of Hanoi Simulation");

	/*if(i==1)
{
disks=3;
}
else if(i==2){
disks=4;
}
else if(i==3){
disks=5;
}*/
	hanoiinit();
	reset();
	atexit(hanoicleanup);
	Init();
	glutReshapeFunc(Reshape);
	speed = 0;
	glutKeyboardFunc(Key);
	//glutSpecialFunc(SpecialKey);
	//glutMouseFunc(mouse);
	glutDisplayFunc(Display);
	glutTimerFunc((unsigned)FEM, moveDisk, 0);
	glutFullScreen();
	glutMainLoop();
}




#include <glut.h>
#include <string.h>
#include<stdio.h>
#include<math.h> 
#include<cmath>


void *currentfont;
void setFont(void *font)
{
    currentfont = font;
}

void delay()
{
	int i,j;
	for(i=0;i<46000;i++)
		for(j=0;j<35000;j++);//delay
}

void draws(float x, float y, float z, char *string)
{

    int len = (int) strlen(string);
    int i;
    glRasterPos2f(x, y);
    for(i = 0; i < len; i++)
    {
        glColor3f(0.0, 0.0, 0.0);
        glutBitmapCharacter(currentfont, string[i]);
    }
}
void background()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBegin(GL_QUADS);
	glColor3f(0.4f,0.8f,0.8f);
	glVertex2f(-100,100);
	glColor3f(0.8f,0.6f,0.8f);
	glVertex2f(-100,-100);
	glColor3f(0.6f,0.8f,0.6f);
	glVertex2f(100,-100);
	glColor3f(0.7f,0.7f,1.0f);
	glVertex2f(100,100);
	glEnd();
}



void display(void)
{

	background();
	//glClearColor(0.4, 0.4, 0.5, 0.0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//setFont(GLUT_BITMAP_HELVATICA_18);
	setFont(GLUT_BITMAP_9_BY_15);

	glColor3f(0.4, 0.0, 0.6); 
	draws(-37, 80,0.0, "SDM INSTITUTE OF TECHNOLOGY, UJIRE- 574240");
	
	glColor3f(0.0, 0.1, 0.5); 
	draws(-40, 70,0.0, "DEPARTMENT OF COMPUTER SCIENCE AND ENGINEERING");

	glColor3f(0.0, 0.0, 0.0); 
	draws(-47, 60,0.0 ,"COMPUTER GRAPHICS LABORATORY WITH MINI PROJECT(18CSL67)");
	
	glColor3f(1.0, 0.0, 0.0); 
	draws(-19, 40,0.0, "A MINI PROJECT ON");
	
	glColor3f(0.0, 0.1, 0.5); 
	draws(-19, 30,0.0, "--TOWER OF HANOI--");
	
    glColor3f(1, 0, 0); 
	draws(-80, 7, 0.0, "SUBMITTED BY:");

	draws(-80, -3, 0.0, "SHETTY NIDHI DINESH  (4SU20CS093)");
	draws(-80, -13, 0.0, "SHIFALI U            (4SU20CS094)");
	draws(-80, -23, 0.0, "SHREYA               (4SU20CS098)");
	draws(-80, -33, 0.0, "SHREYA NAG R C       (4SU20CS099)");

	glColor3f(1, 0, 0); 
	draws(50, 8, 0.0, "GUIDE:");

	draws(50, -2, 0.0, "MR. ARJUN K");
	draws(50, -12, 0.0, "ASSISTANT PROFESSOR");
	draws(50, -22, 0.0, "DEPT OF CSE");

	glColor3f(1, 0, 0); 
	draws(50,-50, 0.0, "PRESS ''X'' TO MENU ");
    glutSwapBuffers();
    glFlush();
}
void myinit()
{
	glClearColor(0.8, 0.1, 0.8, 0.0);
    glOrtho(-100.0, 100.0, -100.0, 100.0, -50.0, 50.0);

}
void menu()
{
	background();
	//glClearColor(0.4, 0.4, 0.5, 0.0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	delay();
    glColor3f(1.0, 0.0, 0.0); 
    draws(-5, 45, 0.0,"MENU");
	glFlush();
	delay();
	glColor3f(1.0, 0.0, 0.0); 
    draws(-5.5, 43, 0.0,"___");
	glFlush();
	delay();
    glColor3f(0.0, 0.0, 0.0);
    draws(-8, 20,0.0, "START(s)");
	glFlush();
	delay();
    glColor3f(0.0, 0.0, 0.0);
    draws(-8, 10,0.0, "ABOUT(a)");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 0.0);
    draws(-8, 0,0.0, "FLOWCHART(f)");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 0.0);
	draws(-8, -10,0.0, "HELP(h)");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 0.0);
	draws(-8, -20,0.0, "INSTRUCTION(i)");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 0.0);
    draws(-8, -30,0.0, "QUIT(q)");
	glFlush();

	delay();
	glBegin(GL_LINE_LOOP);
	glColor3f(0, 0, 0);//white
	glVertex2f(21,-35);
	glVertex2f(-24,-35);
	glVertex2f(-24,30);
	glVertex2f(21,30);
	glEnd();
	glFlush();

	delay();
	glColor3f(1.0, 0.0, 0.0);
    draws(-95,-91,0.0,"<<<PRESS 'D' TO GO BACK");
	glFlush();
	
    glutSwapBuffers();
}


void about()
{
	 background();

	//glClearColor(0.4, 0.4, 0.5, 0.0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	delay();
    glColor3f(1.0,0.0,0.0);
    draws(-5,83,0.0, "ABOUT US");
	glFlush();
    //glColor3f(1.0, 1.0, 1.0);
    //draws(-80,70,0.0, "");
    //draws(50, 370,0.0, "__");
	delay();
    glColor3f(0.0, 0.0, 1.0);
    draws(-80,65,0.0, "Name:Shetty Nidhi Dinesh ");
	glFlush();
	 //glColor3f(1, 0.5, 0.5);
    //draws(-70,50, 0.0," ---------------");
	delay();
    glColor3f(0.0, 0.0, 1.0);
    draws(-80,55,0.0,"USN:4SU20CS093");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
	draws(-80,45,0.0,"Branch:CSE");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(-80,35,0.0,"Contact:9082104373");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(-80,25,0.0,"Address:10/203, Shree Siddhivinayak Park,");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(-70,15,0.0," Lokmanya Nagar, Thane West,");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(-70,5,0.0," Maharashtra");
	glFlush();
	
	delay();
	glBegin(GL_LINE_LOOP);
	glColor3f(0, 0, 0);//white
	glVertex2f(-12,0);
	glVertex2f(-90,0);
	glVertex2f(-90,75);
	glVertex2f(-12,75);
	glEnd();
	glFlush();
	glBegin(GL_LINE_LOOP);
	glColor3f(0, 0, 0);//white
	glVertex2f(-12.5,1);
	glVertex2f(-90.5,1);
	glVertex2f(-90.5,74);
	glVertex2f(-12.5,74);
	glEnd();
	glFlush();


	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(40,65,0.0, "Name:Shifali U");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(40,55,0.0, "USN:4SU20CS094");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(40,45,0.0, "Branch:CSE");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(40,35,0.0,"Contact:8277235001");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(40,25,0.0,"Address:'Shivali', Pragathinagara,"); 
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(53,15,0.0,"Kashibettu, Laila PO,"); 
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(53,5,0.0,"Belthangady  Taluk - 574214");
	glFlush();

	delay();
	glBegin(GL_LINE_LOOP);
	glColor3f(0, 0, 0);//white
	glVertex2f(95,0);
	glVertex2f(20,0);
	glVertex2f(20,75);
	glVertex2f(95,75);
	glEnd();
	glFlush();
	glBegin(GL_LINE_LOOP);
	glColor3f(0, 0, 0);//white
	glVertex2f(95.5,1);
	glVertex2f(20.5,1);
	glVertex2f(20.5,76);
	glVertex2f(95.5,76);
	glEnd();
	glFlush();


	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(-80,-15,0.0, "Name:Shreya ");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(-80,-25.0,0.0 ,"USN:4SU20CS098");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
	draws(-80,-35,0.0, "Branch:CSE");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(-80,-45.0,0.0 ,"Contact:7676150489");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
	draws(-80,-55.0,0.0 ,"Address:#8/4, Hippa house,");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
	draws(-68,-65.0,0.0 ,"Belal Post and Village,");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
	draws(-68,-75.0,0.0 ,"Belthangady Tq,D.K-574240");
	glFlush();

	delay();
	glBegin(GL_LINE_LOOP);
	glColor3f(0, 0, 0);//white
	glVertex2f(-12,-79);
	glVertex2f(-90,-79);
	glVertex2f(-90,-5);
	glVertex2f(-12,-5);
	glEnd();
	glFlush();
	glBegin(GL_LINE_LOOP);
	glColor3f(0, 0, 0);//white
	glVertex2f(-12.5,-80);
	glVertex2f(-90.5,-80);
	glVertex2f(-90.5,-6);
	glVertex2f(-12.5,-6);
	glEnd();
	glFlush();


	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(40,-15,0.0, "Name:Shreya Nag R C  ");
	glFlush();
	delay();
    glColor3f(0.0, 0.0, 1.0);
    draws(40,-25,0.0, "USN:4SU20CS099");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(40,-35,0.0, "Branch:CSE");
	glFlush();
	delay();
    glColor3f(0.0, 0.0, 1.0);
    draws(40,-45,0.0, "Contact:9606428879");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(40,-55,0.0,"Adress:Siddharudha Nagara,"); 
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(50,-65,0.0,"B H Road,Bhadravathi");
	glFlush();
	delay();
	glBegin(GL_LINE_LOOP);
	glColor3f(0, 0, 0);//white
	glVertex2f(95,-79);
	glVertex2f(20,-79);
	glVertex2f(20,-5);
	glVertex2f(95,-5);
	glEnd();
	glFlush();
	glBegin(GL_LINE_LOOP);
	glColor3f(0, 0, 0);//white
	glVertex2f(95.5,-80);
	glVertex2f(20.5,-80);
	glVertex2f(20.5,-6);
	glVertex2f(95.5,-6);
	glEnd();
	glFlush();
	//delay();
	// glColor3f(1, 0.5, 0.5);
	 delay();
	 glColor3f(1.0, 0.0, 0.0);
    draws(-95,-91,0.0,"<<<PRESS 'X' TO GO BACK");
	glFlush();
	delay();
	 glColor3f(1.0, 0.0, 0.0);
    draws(60,-91,0.0,"PRESS 'Q' TO CLOSE>>>");
	 glFlush();
   /* draws(-70,-10,0.0 ,"------------ ");
    glColor3f(0.0, 1.0, 1.0);
    draws(-60,5,0.0 ,"*left click on mouse to go to the previous window");
   //glutKeyboardFunc(keyboard);*/
    //glutSwapBuffers();
   


	



}



void help()
{
	background();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(-10,80,0.0, "TOWER OF HANOI");
	glFlush();
	delay();
	draws(-90,60,0.0,"The Tower of Hanoi (also called The problem of Benares Temple or Tower of Brahma or Lucas' Tower and sometimes pluralized as ");
	glFlush();
	delay();
	draws(-90,50,0.0,"Towers, or simply pyramid puzzle) is a mathematical game or puzzle consisting of three rods and a number of disks of various");
	glFlush();
	delay();
	draws(-90,40,0.0,"diameters, which can slide onto any rod. The puzzle begins with the disks stacked on one rod in order of decreasing size,the");
	glFlush();
	delay();
	draws(-90,30,0.0,"smallest at the top, thus approximating a conical shape. The objective of the puzzle is to move the entire stack to the last");
	glFlush();
	delay();
	draws(-90,20,0.0,"rod.");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
	draws(-90,0,0.0,"Rules:");
	glFlush();
	delay();
	draws(-90,-15,0.0,"1] Only one disk may be moved at a time.");
	glFlush();
	delay();
	draws(-90,-25,0.0,"2] Each move consists of taking the upper disk from one of the stacks and placing it on top of another stack or on an empty");
	glFlush();
	delay();
	draws(-90,-35,0.0,"   rod.");
	glFlush();
	delay();
	draws(-90,-45,0.0,"3] No disk may be placed on top of a disk that is smaller than it.");
	glFlush();
	delay();
	draws(-90,-55,0.0,"4] With 3 disks, the puzzle can be solved in 7 moves. The minimal number of moves required to solve a Tower of Hanoi puzzle");
	glFlush();
	delay();
	draws(-90,-65,0.0,"   is 2n - 1, where n is the number of disks.");
	glFlush();
	delay();
	glColor3f(1.0, 0.0, 0.0);
    draws(-95,-91,0.0,"<<<PRESS 'X' TO GO BACK");
	glFlush();
	delay();
	glColor3f(1.0, 0.0, 0.0);
    draws(60,-91,0.0,"PRESS 'Q' TO CLOSE>>>");
	glFlush();
}



void info()

{
	background();
	//glClearColor(0.4, 0.4, 0.5, 0.0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	delay();
    glColor3f(0.5, 0.0, 0.5);
    draws(-10,80,0.0, "TOWER OF HANOI");
	glFlush();
	delay();
    glColor3f(0.0, 0.2, 0.0);
    draws(-80,70,0.0, "INSTRUCTIONS:");
	glFlush();
	delay();
    //draws(50, 370, "__");
    glColor3f(0.0, 0.0, 0.0);
    draws(-70,60,0.0, "Keyboard Functions");
	glFlush();
	delay();
	 //glColor3f(1, 0.5, 0.5);
    //draws(-70,50, 0.0," ---------------");
    glColor3f(0.0, 0.0, 1.0);
    draws(-60,50,0.0, "-> Press 'X' to start");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(-60,45,0.0, "-> Press 'n' to enter the values");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(-60,40,0.0, "-> Press 'h' for help window");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(-60,35,0.0, "-> Press 'q' for quit");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(-60,30,0.0, "-> Press 'm' for main menu");
	glFlush();
	delay();
	glColor3f(0.0, 0.0, 1.0);
    draws(-60,25,0.0, "-> press any key to close the terminal");
	glFlush();
	delay();
    glColor3f(0.0, 0.0, 0.0);
    draws(-70,15,0.0, "Mouse Function");
	glFlush();
	delay();
	 //glColor3f(1, 0.5, 0.5);
   // draws(-70,-10,0.0 ,"------------ ");
    glColor3f(0.0, 0.0, 1.0);
    draws(-60,5,0.0 ,"->left click on mouse to go to the previous window");
	glFlush();
	delay();
   //glutKeyboardFunc(keyboard);
    //glutSwapBuffers();

	 glColor3f(1.0, 0.0, 0.0);
    draws(-95,-91,0.0,"<<<PRESS 'X' TO GO BACK");
	glFlush();
	delay();
	 glColor3f(1.0, 0.0, 0.0);
    draws(60,-91,0.0,"PRESS 'Q' TO CLOSE>>>");

    glFlush();
	}




void flowchart()
{
	int i;

	//glClearColor(0.1, 0.0, 0.4, 0.0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	background();

	delay();
	glColor3f(0.0, 0.2, 0.0);
	draws(-22, 90, 0.0, "FLOW CHART OF TOWER OF HANOI");
	glFlush();




	glColor3f(1.0f, 1.0f, 1.0f); // Set color to blue
    
    const float PI = 3.14159f;
    const int numSegments = 100;
    const float centerX = 0.1f;
    const float centerY = 75.0f;
    const float radiusX = 7.5f;
    const float radiusY = 7.5f;
    
	delay();
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(centerX, centerY);
    for (int i = 0; i <= numSegments; ++i) {
        float theta = 2.0f * PI * static_cast<float>(i) / static_cast<float>(numSegments);
        float x = centerX + radiusX * std::cos(theta);
        float y = centerY + radiusY * std::sin(theta);
        glVertex2f(x, y);
	
    }
    glEnd();
	glFlush();

		delay();
	glColor3f(0,0,0);
	draws(-3.5, 73,0.0, "START");
	glFlush();

	delay();
	glBegin(GL_LINES);
	glVertex2f(0,68);
	glVertex2f(0,60);
	glEnd();
	glFlush();

	delay();
	glBegin(GL_TRIANGLES);
	glColor3f(0, 0, 0);//black
	glVertex2f(-0.5,63);
	glVertex2f(0,60);
	glVertex2f(0.5,63);
	glEnd();
	glFlush();




	delay();
	glBegin(GL_POLYGON);
	glColor3f(1, 1, 1);//white
	glVertex2f(20,60);
	glVertex2f(-23,60);
	glVertex2f(-25,40);
	glVertex2f(17,40);
	glEnd();
	delay();
	glColor3f(0, 0, 0);
	draws(-19, 50,0.0,  "Three towers are assumed ");
	delay();
	glColor3f(0,0,0);
	draws(-19, 45,0.0,"to be aux,dest,source");
	glFlush();

	delay();
	glBegin(GL_LINES);
	glVertex2f(0,40);
	glVertex2f(0,30);
	glEnd();
	glFlush();

	delay();
	glBegin(GL_TRIANGLES);
	glColor3f(0, 0, 0);//black
	glVertex2f(-0.5,33);
	glVertex2f(0,30);
	glVertex2f(0.5,33);
	glEnd();
	glFlush();

	delay();
	glBegin(GL_POLYGON);
	glColor3f(1, 1, 1);//white
	glVertex2f(17,30);
	glVertex2f(-25,30);
	glVertex2f(-26,20);
	glVertex2f(15,20);
	glEnd();

	delay();
	glColor3f(0, 0, 0);
	draws(-7, 25,0.0,  "Read n");
	glFlush();


    delay();
	glBegin(GL_LINES);
	glVertex2f(0,20);
	glVertex2f(0,10);
	glEnd();
	glFlush();
	delay();

	delay();
	glBegin(GL_TRIANGLES);
	glColor3f(0, 0, 0);//black
	glVertex2f(-0.5,13);
	glVertex2f(0,10);
	glVertex2f(0.5,13);
	glEnd();
	glFlush();
	

		




	glBegin(GL_POLYGON);
	glColor3f(1, 1, 1);//white
	glVertex2f(0,10);//1st point
	glVertex2f(-20,0);//2nd point
	glVertex2f(0,-10);
	glVertex2f(20,-0);
	glEnd();
	
	delay();
	glColor3f(0, 0, 0);
	draws(-1.5, -0,0.0,  "n>0");
	glFlush();

	delay();
	glBegin(GL_LINES);
	glVertex2f(0,-10);
	glVertex2f(0,-20);
	glEnd();
	glFlush();

	delay();
	glBegin(GL_TRIANGLES);
	glColor3f(0, 0, 0);//black
	glVertex2f(-0.5,-17);
	glVertex2f(0,-20);
	glVertex2f(0.5,-17);
	glEnd();
	glFlush();
	glColor3f(0, 0, 0);
	draws(5, -15,0.0, "yes");
	glFlush();


	delay();
	glBegin(GL_POLYGON);
	glColor3f(1, 1, 1);//white
	glVertex2f(21,-30);
	glVertex2f(-26,-30);
	glVertex2f(-26,-20);
	glVertex2f(21,-20);
	glEnd();
	delay();
	glColor3f(0, 0, 0);
	draws(-25, -25,0.0,  "Move n-1 disk from source to aux");
	glFlush();

	delay();
	glBegin(GL_LINES);
	glVertex2f(0,-30);
	glVertex2f(0,-40);
	glEnd();
	glFlush();

	delay();
	glBegin(GL_TRIANGLES);
	glColor3f(0, 0, 0);//black
	glVertex2f(-0.5,-37);
	glVertex2f(0,-40);
	glVertex2f(0.5,-37);
	glEnd();
	glFlush();


	delay();
	glBegin(GL_POLYGON);
	glColor3f(1, 1, 1);//white
	glVertex2f(22,-50);
	glVertex2f(-26,-50);
	glVertex2f(-26,-40);
	glVertex2f(22,-40);
	glEnd();

	delay();
	glColor3f(0, 0, 0);
	draws(-25, -45,0.0, "Move nth disk from source to dest");
	glFlush();

	delay();
	glBegin(GL_LINES);
	glVertex2f(0,-50);
	glVertex2f(0,-60);
	glEnd();
	glFlush();

	
		delay();
	glBegin(GL_TRIANGLES);
	glColor3f(0, 0, 0);//black
	glVertex2f(-0.5,-57);
	glVertex2f(0,-60);
	glVertex2f(0.5,-57);
	glEnd();
	glFlush();



		delay();
	glBegin(GL_POLYGON);
	glColor3f(1, 1, 1);//white
	glVertex2f(22,-70);
	glVertex2f(-26,-70);
	glVertex2f(-26,-60);
	glVertex2f(22,-60);
	glEnd();
		delay();
	glColor3f(0, 0, 0);
	draws(-25, -65,0.0, "Move n-1 disk from aux to dest");
	glFlush();

		delay();
	glBegin(GL_LINES);
	glVertex2f(0,-70);
	glVertex2f(0,-80);
	glEnd();
	glFlush();
		delay();

		delay();
	glBegin(GL_TRIANGLES);
	glColor3f(0, 0, 0);//black
	glVertex2f(-0.5,-77);
	glVertex2f(0,-80);
	glVertex2f(0.5,-77);
	glEnd();
	glFlush();



    glBegin(GL_POLYGON);
	glColor3f(1, 1, 1);//white
	glVertex2f(10,-90);
	glVertex2f(-10,-90);
	glVertex2f(-10,-80);
	glVertex2f(10,-80);
	glEnd();
		delay();
	glColor3f(0, 0, 0);
	draws(-3.5, -86,0.0, "n=n-1");
	glFlush();

		delay();
	glBegin(GL_LINES);
	glVertex2f(0,-90);
	glVertex2f(0,-97);
	glEnd();
	glFlush();

	delay();
	glBegin(GL_TRIANGLES);
	glColor3f(0, 0, 0);//black
	glVertex2f(-22,-0.8);
	glVertex2f(-20,0);
	glVertex2f(-22,0.8);
	glEnd();
	glFlush();


		delay();
	glBegin(GL_LINES);
	glVertex2f(0,-97);
	glVertex2f(-45,-97);
	glEnd();
	glFlush();
		delay();
	glBegin(GL_LINES);
	glVertex2f(-45,-97);
	glVertex2f(-45,0);
	glEnd();
	glFlush();
		delay();
	glBegin(GL_LINES);
	glVertex2f(-45,0);
	glVertex2f(-20,0);
	glEnd();
	glFlush();
		delay();
	glBegin(GL_LINES);
	glVertex2f(20,0);
	glVertex2f(35,0);
	glEnd();
	glFlush();
	
		delay();
	glBegin(GL_TRIANGLES);
	glColor3f(0, 0, 0);//black
	glVertex2f(33,-0.9);
	glVertex2f(35,0);
	glVertex2f(33,0.9);
	glEnd();
	glFlush();

	//Ovals
	

	
	//Ovals
	glColor3f(1.0f, 1.0f, 1.0f); // Set color to blue
    
    const float PIi = 3.14159f;
    const int numbSegments = 100;
    const float centerXx = 42.0f;
    const float centerYy = 0.0f;
    const float radiusXx = 7.5f;
    const float radiusYy = 7.5f;
    
		delay();
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(centerXx, centerYy);
    for (int i = 0; i <= numbSegments; ++i) {
        float theta = 2.0f * PIi * static_cast<float>(i) / static_cast<float>(numbSegments);
        float xx = centerXx + radiusXx * std::cos(theta);
        float yy = centerYy + radiusYy * std::sin(theta);
        glVertex2f(xx, yy);
	
    }
    glEnd();
	glFlush();

	glColor3f(0, 0, 0);
	draws(25, 5,0.0, "no");
	glFlush();

	

		delay();
	glColor3f(0,0,0);
	draws(40,-1.0,0.0, "END");
	glFlush();

		delay();
	 glColor3f(1.0, 0.0, 0.0);
    draws(-95,-91,0.0,"<<<PRESS 'X' TO GO BACK");
	glFlush();
		delay();
	 glColor3f(1.0, 0.0, 0.0);
    draws(60,-91,0.0,"PRESS 'Q' TO CLOSE>>>");

	glFlush();


	//drawCircle(0,0,4,8);
	//drawOval(0,60,10,10,4);

	


}



/*void start()
{

 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	background();

	
	glColor3f(0.0, 0.0, 0.0);
    draws(-20,80,0.0,"<<<DESIGN OF TOWER OF HANOI");


	glBegin(GL_POLYGON);
	glColor3f(1, 1, 1);//white
	glVertex2f(-26,0);
	glVertex2f(-26,-50);
	glVertex2f(-30,-50);
	glVertex2f(-30,0);
	glEnd();
	glFlush();


	glBegin(GL_POLYGON);
	glColor3f(1, 1, 1);//white
	glVertex2f(-6,0);
	glVertex2f(-6,-50);
	glVertex2f(-2,-50);
	glVertex2f(-2,0);
	glEnd();
	glFlush();


	glBegin(GL_POLYGON);
	glColor3f(1, 1, 1);//white
	glVertex2f(15,0);
	glVertex2f(15,-50);
	glVertex2f(19,-50);
	glVertex2f(19,0);
	glEnd();
	glFlush();



}*/

void keyboard(unsigned char key, int a, int b) {
    if (key == 'X' || key == 'x') {

        glClearColor(1.0, 3.0, 6.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
        menu();

    }
	if (key == 'D' || key == 'd') {

        glClearColor(1.0, 3.0, 6.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
        display();

    }
    if (key == 'h' || key == 'H') {
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
        help();
    }
    if (key == 's' || key == 'S') {
        glClear(GL_COLOR_BUFFER_BIT);
        tower();

    }
    if (key == 'q' || key == 'Q')
    {
        exit(0);
    }
    if (key == 'f' || key == 'f') {
        flowchart();
    }
    if (key == 'a' || key == 'A') {
        about();
    }
	 if (key == 'i' || key == 'I') {
        glClear(GL_COLOR_BUFFER_BIT);
        info();

    }

}

void mouses(int btn,int state,int x,int y)
{
	if(btn==GLUT_LEFT_BUTTON && state==GLUT_DOWN)
	{
		menu();
	}
	if(btn==GLUT_RIGHT_BUTTON && state==GLUT_DOWN)
	{
		exit(0);
	}
}





int main(int argc, char**argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(1500, 1500);
    glutCreateWindow("TOWER OF HANOI");
	myinit();
    glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouses);
	glutFullScreen();
    glutMainLoop();
	return 0;

}