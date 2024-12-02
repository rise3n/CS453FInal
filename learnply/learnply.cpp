#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <fstream>
#include <vector>

#include "glError.h"
#include "GL/glew.h"
#include "GL/freeglut.h"
#include "ply.h"
#include "icVector.H"
#include "icMatrix.H"
#include "polyhedron.h"
#include "trackball.h"
#include "tmatrix.h"
#include "ibfv.h"
#include "drawPoly.h"
#include "drawUtil.h"
#include <iostream>
#include "project1.h"
#include "project2.h"
#include "LTS.h"

Polyhedron* poly = NULL;
IBFV* ibfv = NULL;
ScalarTopology* stopo = NULL;

/*scene related variables*/
const float zoomspeed = 0.9;
int win_width = 1024;
int win_height = 1024;
float aspectRatio = win_width / win_height;
const int view_mode = 0;		// 0 = othogonal, 1=perspective
const double radius_factor = 0.9;

/*
Use keys 1 to 0 to switch among different display modes.
Each display mode can be designed to show one type 
visualization result.

display mode 1: solid rendering
display mode 2: wireframe rendering
display mode 3: image based flow visualization (IBFV)
display mode 4: grayscale color scheme
display mode 5: bi-color color scheme
display mode 6: rainbow heat map color scheme
display mode 7-?: your visualizations
*/
int display_mode = 1;

// toggle height field with the 'h' key
bool use_height = false;
bool draw_stopo = false;

/*User Interaction related variabes*/
float s_old, t_old;
float rotmat[4][4];
double zoom = 1.0;
double translation[2] = { 0, 0 };
int mouse_mode = -2;	// -1 = no action, 1 = tranlate y, 2 = rotate


/******************************************************************************
Forward declaration of functions
******************************************************************************/

void init(void);
void set_view(GLenum mode);
void set_scene(GLenum mode, Polyhedron* poly);
int processHits(GLint hits, GLuint buffer[]);
void display(void);

/*glut attaching functions*/
void keyboard(unsigned char key, int x, int y);
void motion(int x, int y);
void display(void);
void mouse(int button, int state, int x, int y);
void mousewheel(int wheel, int direction, int x, int y);
void special(int key, int x, int y);
void reshape(int width, int height);
std::vector<CriticalPoint> extremesplit(std::vector<CriticalPoint> criticals);
std::vector<CriticalPoint> randomReserve(std::vector<CriticalPoint> extreme);

/******************************************************************************
Main program.
******************************************************************************/

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("Usage: learnply <file>\n");
		return 1;
	}

	/*load mesh from ply file*/
	FILE* this_file = fopen(argv[1], "r");
	poly = new Polyhedron(this_file);
	fclose(this_file);
	
	/*initialize the mesh*/
	poly->initialize(); // initialize the mesh
	poly->write_info();

	//initialize scalar file topology
	stopo = new ScalarTopology(poly);

	/*init glut and create window*/
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowPosition(20, 20);
	glutInitWindowSize(win_width, win_height);
	glutCreateWindow("Scientific Visualization");

	/*initialize openGL*/
	init();
	//remaining hw code
	stopo->color_scheme = COLOR_SCHEME::RAINBOW;

	std::vector<CriticalPoint> criticals = stopo->getcriticals();
	std::vector<CriticalPoint> extremes = extremesplit(criticals);

	//say randomly reserve some extreme points
	std::vector<CriticalPoint> reserved_extreme = randomReserve(extremes);

	LocalSimplification* LTS = new LocalSimplification(extremes, reserved_extreme, poly);
	std::vector<Vertex> reordered_sequence = LTS->LocalTopologicalSimplification();
	//std::vector<Vertex> merged_sequence = LTS->merge(poly->vlist,reordered_sequence);//not sure if we should use vlist here, use it for now

	//Polyhedron* newpoly = poly.deepcopy(); // I want a copy of poly here, leave this for now

	//visualize newpoly here

	/*the render function and callback registration*/
	glutKeyboardFunc(keyboard);
	glutDisplayFunc(display);
	glutMotionFunc(motion);
	glutMouseFunc(mouse);
#ifdef _WIN32
	glutMouseWheelFunc(mousewheel);
#endif //_WIN32
	glutSpecialFunc(special);
	glutReshapeFunc(reshape);

	/*event processing loop*/
	glutMainLoop();
	
	/*clear memory before exit*/
	poly->finalize();	// finalize everything
	if (ibfv)
	{
		ibfv->freepixels();
		delete(ibfv);
	}
	delete(stopo);
	delete(poly);

	return 0;
}


/******************************************************************************
Init scene
******************************************************************************/

void init(void) {

	mat_ident(rotmat);

	/* select clearing color */
	glClearColor(0.0, 0.0, 0.0, 0.0);  // background
	glShadeModel(GL_FLAT);
	glPolygonMode(GL_FRONT, GL_FILL);

	glDisable(GL_DITHER);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	//set pixel storage modes
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	glEnable(GL_NORMALIZE);
	if (poly->orientation == 0)
		glFrontFace(GL_CW);
	else
		glFrontFace(GL_CCW);
}


/******************************************************************************
Set projection mode
******************************************************************************/

void set_view(GLenum mode)
{
	GLfloat light_ambient0[] = { 0.3, 0.3, 0.3, 1.0 };
	GLfloat light_diffuse0[] = { 0.7, 0.7, 0.7, 1.0 };
	GLfloat light_specular0[] = { 0.0, 0.0, 0.0, 1.0 };

	GLfloat light_ambient1[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat light_diffuse1[] = { 0.5, 0.5, 0.5, 1.0 };
	GLfloat light_specular1[] = { 0.0, 0.0, 0.0, 1.0 };

	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular0);

	glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse1);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular1);


	glMatrixMode(GL_PROJECTION);
	if (mode == GL_RENDER)
		glLoadIdentity();

	/*if (view_mode == 0)
		glOrtho(-radius_factor * zoom, radius_factor * zoom, -radius_factor * zoom, radius_factor * zoom, 0.0, 40.0);
	else
		glFrustum(-radius_factor * zoom, radius_factor * zoom, -radius_factor, radius_factor, -1000, 1000);*/

	if (aspectRatio >= 1.0) 
	{
		if (view_mode == 0)
			glOrtho(-radius_factor * zoom * aspectRatio, radius_factor * zoom * aspectRatio, -radius_factor * zoom, radius_factor * zoom, -1000, 1000);
		else
			glFrustum(-radius_factor * zoom * aspectRatio, radius_factor * zoom * aspectRatio, -radius_factor * zoom, radius_factor * zoom, 0.1, 1000);
	}
	else 
	{
		if (view_mode == 0)
			glOrtho(-radius_factor * zoom, radius_factor * zoom, -radius_factor * zoom / aspectRatio, radius_factor * zoom / aspectRatio, -1000, 1000);
		else
			glFrustum(-radius_factor * zoom, radius_factor * zoom, -radius_factor * zoom / aspectRatio, radius_factor * zoom / aspectRatio, 0.1, 1000);
	}

	GLfloat light_position[3];
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	light_position[0] = 5.5;
	light_position[1] = 0.0;
	light_position[2] = 0.0;
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	light_position[0] = -0.1;
	light_position[1] = 0.0;
	light_position[2] = 0.0;
	glLightfv(GL_LIGHT2, GL_POSITION, light_position);
}


/******************************************************************************
Update the scene
******************************************************************************/

void set_scene(GLenum mode, Polyhedron* poly)
{
	glTranslatef(translation[0], translation[1], -3.0);

	/*multiply rotmat to current mat*/
	{
		int i, j, index = 0;

		GLfloat mat[16];

		for (i = 0; i < 4; i++)
			for (j = 0; j < 4; j++)
				mat[index++] = rotmat[i][j];

		glMultMatrixf(mat);
	}

	glScalef(0.9 / poly->radius, 0.9 / poly->radius, 0.9 / poly->radius);
	glTranslatef(-poly->center.entry[0], -poly->center.entry[1], -poly->center.entry[2]);
}


/******************************************************************************
Pick objects from the scene
******************************************************************************/

int processHits(GLint hits, GLuint buffer[])
{
	unsigned int i, j;
	GLuint names, * ptr;
	double smallest_depth = 1.0e+20, current_depth;
	int seed_id = -1;
	unsigned char need_to_update;

	ptr = (GLuint*)buffer;
	for (i = 0; i < hits; i++) {  /* for each hit  */
		need_to_update = 0;
		names = *ptr;
		ptr++;

		current_depth = (double)*ptr / 0x7fffffff;
		if (current_depth < smallest_depth) {
			smallest_depth = current_depth;
			need_to_update = 1;
		}
		ptr++;
		current_depth = (double)*ptr / 0x7fffffff;
		if (current_depth < smallest_depth) {
			smallest_depth = current_depth;
			need_to_update = 1;
		}
		ptr++;
		for (j = 0; j < names; j++) {  /* for each name */
			if (need_to_update == 1)
				seed_id = *ptr - 1;
			ptr++;
		}
	}
	return seed_id;
}


/******************************************************************************
Process a keyboard action.  In particular, exit the program when an
"escape" is pressed in the window.
******************************************************************************/

void keyboard(unsigned char key, int x, int y) 
{
	switch (key) 
	{
	case 27: // escape key to exit program
	{
		poly->finalize();  // finalize_everything
		exit(0);
	}
	break;
	case 'r': // reset camera viewpoint
	{
		mat_ident(rotmat);
		translation[0] = 0;
		translation[1] = 0;
		zoom = 1.0;
		glutPostRedisplay();
	}
	break;
	case 'h': // toggle height field
	{
		use_height = !use_height;
		if (use_height)
			height(poly);
		else
			resetHeight(poly);
		glutPostRedisplay();
	}
	break;
	case 's': // scalar topology vis
	{
		draw_stopo = !draw_stopo;
		glutPostRedisplay();

	}
	break;
	case 'c': // draw a contour at a user specified value
	{
		double s = 0.0;
		std::cout << "enter a scaler value:";
		std::cin >> s;
		stopo->calcUserContour(s);
		draw_stopo = true;
		glutPostRedisplay();


	}
	break;
	case 'n': // draw evenly spaced contours given a input n
	{
		int n = 0;
		std::cout << "enter a number of evenly spaced contours:";
		std::cin >> n;
		stopo->calcSpacedContours(n);
		draw_stopo = true;
		glutPostRedisplay();
	}
	break;
	case '1': // solid rendering
	{
		display_mode = 1;
		glutPostRedisplay();
	}
	break;
	case '2': // wireframe rendering
	{
		display_mode = 2;
		glutPostRedisplay();
	}
	break;
	case '3': // grayscale color scheme for scalar field vis
	{
		display_mode = 3;
		greyscale(poly);
		glutPostRedisplay();
	}
	break;
	case '4': // bi-color color scheme for scalar field vis
	{
		display_mode = 4;
		multi(poly);
		glutPostRedisplay();
	}
	break;
	case '5': // rainbow heat map color scheme for scalar field vis
	{
		display_mode = 5;
		heatmap(poly);
		glutPostRedisplay();
	}
	break;
	case '6':
	{
		display_mode = 6;
		stopo->color_scheme = COLOR_SCHEME::SOLID;
		glutPostRedisplay();
	}
	break;
	case '7':
	{
		display_mode = 7;
		stopo->color_scheme = COLOR_SCHEME::GRAYSCALE;
		glutPostRedisplay();
	}
	break;
	case '8':
	{
		display_mode = 8;
		stopo->color_scheme = COLOR_SCHEME::BICOLOR;
		glutPostRedisplay();
	}
	break;
	case '9':
	{
		display_mode = 9;
		stopo->color_scheme = COLOR_SCHEME::RAINBOW;
		glutPostRedisplay();
	}
	break;
	default:
		glutPostRedisplay();
		break;
	}
}


/******************************************************************************
Callback function for dragging mouse
******************************************************************************/

void motion(int x, int y) {
	float r[4];
	float s, t;

	s = (2.0 * x - win_width) / win_width;
	t = (2.0 * (win_height - y) - win_height) / win_height;

	if ((s == s_old) && (t == t_old))
		return;

	switch (mouse_mode) {
	case 2:

		Quaternion rvec;

		mat_to_quat(rotmat, rvec);
		trackball(r, s_old, t_old, s, t);
		add_quats(r, rvec, rvec);
		quat_to_mat(rvec, rotmat);

		s_old = s;
		t_old = t;

		display();
		break;

	case 1:

		translation[0] += (s - s_old);
		translation[1] += (t - t_old);

		s_old = s;
		t_old = t;

		display();
		break;
	}
}


/******************************************************************************
Callback function for mouse clicks
******************************************************************************/

void mouse(int button, int state, int x, int y) {

	int key = glutGetModifiers();

	if (button == GLUT_LEFT_BUTTON || button == GLUT_RIGHT_BUTTON) {
		
		if (state == GLUT_DOWN) {
			float xsize = (float)win_width;
			float ysize = (float)win_height;

			float s = (2.0 * x - win_width) / win_width;
			float t = (2.0 * (win_height - y) - win_height) / win_height;

			s_old = s;
			t_old = t;

			/*translate*/
			if (button == GLUT_LEFT_BUTTON)
			{
				mouse_mode = 1;
			}

			/*rotate*/
			if (button == GLUT_RIGHT_BUTTON)
			{
				mouse_mode = 2;
			}
		}
		else if (state == GLUT_UP) {

			if (button == GLUT_LEFT_BUTTON && key == GLUT_ACTIVE_SHIFT) {  // build up the selection feedback mode

				/*select face*/

				GLuint selectBuf[512];
				GLint hits;
				GLint viewport[4];

				glGetIntegerv(GL_VIEWPORT, viewport);

				glSelectBuffer(512, selectBuf);
				(void)glRenderMode(GL_SELECT);

				glInitNames();
				glPushName(0);

				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();

				/*create 5x5 pixel picking region near cursor location */
				gluPickMatrix((GLdouble)x, (GLdouble)(viewport[3] - y), 1.0, 1.0, viewport);

				set_view(GL_SELECT);
				set_scene(GL_SELECT, poly);
				display_quads(GL_SELECT, poly);

				glMatrixMode(GL_PROJECTION);
				glPopMatrix();
				glFlush();

				glMatrixMode(GL_MODELVIEW);

				hits = glRenderMode(GL_RENDER);
				poly->selected_quad = processHits(hits, selectBuf);
				printf("Selected quad id = %d\n", poly->selected_quad);
				glutPostRedisplay();

				CHECK_GL_ERROR();

			}
			else if (button == GLUT_LEFT_BUTTON && key == GLUT_ACTIVE_CTRL)
			{
				/*select vertex*/

				GLuint selectBuf[512];
				GLint hits;
				GLint viewport[4];

				glGetIntegerv(GL_VIEWPORT, viewport);

				glSelectBuffer(512, selectBuf);
				(void)glRenderMode(GL_SELECT);

				glInitNames();
				glPushName(0);

				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();

				/*  create 5x5 pixel picking region near cursor location */
				gluPickMatrix((GLdouble)x, (GLdouble)(viewport[3] - y), 1.0, 1.0, viewport);

				set_view(GL_SELECT);
				set_scene(GL_SELECT, poly);
				display_vertices(GL_SELECT, poly);

				glMatrixMode(GL_PROJECTION);
				glPopMatrix();
				glFlush();

				glMatrixMode(GL_MODELVIEW);

				hits = glRenderMode(GL_RENDER);
				poly->selected_vertex = processHits(hits, selectBuf);
				printf("Selected vert id = %d\n", poly->selected_vertex);
				glutPostRedisplay();

			}

			mouse_mode = -1;
		}
	}
}


/******************************************************************************
Callback function for mouse wheel scroll
******************************************************************************/

void mousewheel(int wheel, int direction, int x, int y) {
	if (direction == 1) {
		zoom *= zoomspeed;
		glutPostRedisplay();
	}
	else if (direction == -1) {
		zoom /= zoomspeed;
		glutPostRedisplay();
	}
}


/******************************************************************************
Callback function for special keys (e.g. arrow keys)
******************************************************************************/

void special(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_UP:
		mousewheel(0, 1, 0, 0);
		break;
	case GLUT_KEY_DOWN:
		mousewheel(0, -1, 0, 0);
		break;
	}
}


/******************************************************************************
Callback function for window reshaping
******************************************************************************/

void reshape(int width, int height)
{
	win_width = width;
	win_height = height;

	aspectRatio = (float)width / (float)height;

	glViewport(0, 0, width, height);

	set_view(GL_RENDER);
}


/******************************************************************************
Callback function for scene display
******************************************************************************/

void display(void)
{
	glClearColor(1.0, 1.0, 1.0, 1.0);  // background for rendering color coding and lighting

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	set_view(GL_RENDER);
	set_scene(GL_RENDER, poly);

	switch (display_mode)
	{
	case 1:
		drawSolid(poly);
		break;
	case 2:
		drawWirefriame(poly);
		break;
	case 3:
	case 4:
	case 5:
		drawVertColors(poly);
		break;
	case 6:
	case 7:
	case 8:
	case 9:
		drawSolid(poly);
		break;
	default:
		break;
	}

	if (draw_stopo) {
		stopo->drawCriticalPoints(use_height);
		stopo->drawCriticalContours(use_height);
		stopo->drawSpacedContours(use_height);
		stopo->drawUserContour(use_height);
	}



	// display selected elements
	drawSelectedQuad(poly);
	drawSelectedVertex(poly);

	glFlush();
	glutSwapBuffers();
	glFinish();

	CHECK_GL_ERROR();
}


std::vector<CriticalPoint> extremesplit(std::vector<CriticalPoint> criticals)
{
	std::vector<CriticalPoint> extremes;
	for (int i = 0; i < criticals.size(); i++) {
		if (criticals[i].type != SADDLE) {
			extremes.push_back(criticals[i]);
		}
	}
	return extremes;
}

std::vector<CriticalPoint> randomReserve(std::vector<CriticalPoint> extreme)
{
	int num_reserve = floor(0.5 * extreme.size());
	std::vector<CriticalPoint> reserved_extreme;

	for (int i = 0; i < num_reserve; i++) {
		int index = rand() % extreme.size();
		reserved_extreme.push_back(extreme[i]);
	}
	return reserved_extreme;
}