#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>


#ifdef _WIN32
#define drand48() ((float)rand()/RAND_MAX) /*The drand48() functions generate uniformly distributed pseudo-random numbers using a linear congruential algorithm and 48-bit integer arithmetic.*/
#endif


/* #define SCREEN_SAVER_MODE */

#define PS_GRAVITY -9.8
#define PS_WATERFALL  2
#define PS_FOUNTAIN   5


typedef struct {
    float x, y, z;
    float radius;
} PSsphere;

typedef struct {
    float position[3];			/* current position */
    float previous[3];			/* previous position */
    float velocity[3];			/* velocity (magnitude & direction) */
    float dampening;			/* % of energy lost on collision */
    int alive;				/* is this particle alive? */
} PSparticle;


PSparticle* particles = NULL;
#define NUM_SPHERES 5
PSsphere spheres[NUM_SPHERES] = {	/* position of spheres */
    { -0.5, 0.9, 0, 0.2 },
    { -1.7, 1.4, 0, 0.35 },
    { 0.6, 1.5, 0.1, 0.43 },
};
int num_particles = 1000000;
int type = PS_WATERFALL;
int points = 1;
int draw_spheres = 1;
int frame_rate = 1;
float frame_time = 0;
float flow = 500;/* increase the flowrate */
float slow_down = 1;

float spin_x = 0;
float spin_y = 0;
int point_size = 2;
#if defined(_WIN32)
#include <sys/timeb.h>/*The sys/timeb.h header file contains additional definitions for date and time.*/
                       /*It includesunsigned short millitm  the milliseconds portion of the current time*/
#else
#include <unistd.h>   /* unistd.h header defines miscellaneous symbolic constants and types, and declares miscellaneous functions. */
#include <sys/types.h>/*The sys/types.h header file defines a collection of typedef symbols and structures.*/
#include <sys/times.h>/*The sys/times.h header file declares the times() function that gets processor times for use by processes. */
#endif
#ifndef CLK_TCK/*This is an obsolete name for CLOCKS_PER_SEC*/
#define CLK_TCK 1000
#endif
float
timedelta(void)
{
    static long begin = 0;
    static long finish, difference;

#if defined(_WIN32)
    static struct timeb tb;
    ftime(&tb);
    finish = tb.time*100+tb.millitm;
#else
    static struct tms tb;
    finish = times(&tb);
#endif

    difference = finish - begin;
    begin = finish;

    return (float)difference/(float)CLK_TCK;
}
void
text(int x, int y, char* s) 
{
    int lines;
    char* p;

    glDisable(GL_DEPTH_TEST);/*If enabled, do depth	comparisons and update the depth buffer.*/
    glDisable(GL_FOG);
    glMatrixMode(GL_PROJECTION);/* specify which matrix is the current matrix */
    glPushMatrix();/*pushes the current matrix stack down by one, duplicating the current matrix*/
    glLoadIdentity();/*glLoadIdentity replaces the current matrix with the identity matrix. It is semantically equivalent to calling glLoadMatrix with the identity matrix*/
    glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), 
	    0, glutGet(GLUT_WINDOW_HEIGHT), -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor3f(0, 0, 0);
    glRasterPos2i(x+1, y-1);/* specify the raster position for pixel operations*/
    for(p = s, lines = 0; *p; p++) {
	if (*p == '\n') {
	    lines++;
	    glRasterPos2i(x+1, y-1-(lines*18));
	}
	glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
    glColor3f(1.0, 0.0, 0.0);
    glRasterPos2i(x, y);
    for(p = s, lines = 0; *p; p++) {
	if (*p == '\n') {
	    lines++;
	    glRasterPos2i(x, y-(lines*18));
	}
	glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_FOG);
    glEnable(GL_DEPTH_TEST);
}


int
fequal(float a, float b)
{
    float epsilon = 0.1;
    float f = a - b;
    
    if (f < epsilon && f > -epsilon)
	return 1;
    else
	return 0;
}


void
psTimeStep(PSparticle* p, float dt)
{
    if (p->alive == 0)
	return;

    p->velocity[0] += 0;
    p->velocity[1] += PS_GRAVITY*dt;
    p->velocity[2] += 0;

    p->previous[0] = p->position[0];
    p->previous[1] = p->position[1];
    p->previous[2] = p->position[2];

    p->position[0] += p->velocity[0]*dt;
    p->position[1] += p->velocity[1]*dt;
    p->position[2] += p->velocity[2]*dt;
}


void
psNewParticle(PSparticle* p, float dt)
{
    if (type == PS_WATERFALL) {
	p->velocity[0] = -2*(drand48()-0.0);
	p->velocity[1] = 0;
	p->velocity[2] = 0.5*(drand48()-0.0);
	p->position[0] = 0;
	p->position[1] = 2;
	p->position[2] = 0;
	p->previous[0] = p->position[0];
	p->previous[1] = p->position[1];
	p->previous[2] = p->position[2];
	p->dampening = 0.45*drand48();
	p->alive = 1;
    } else if (type == PS_FOUNTAIN) {
	p->velocity[0] = 2*(drand48()-0.5);
	p->velocity[1] = 5;
	p->velocity[2] = 2*(drand48()-0.5);
	p->position[0] = -0.1;
	p->position[1] = 0.9;
	p->position[2] = 0;
	p->previous[0] = p->position[0];
	p->previous[1] = p->position[1];
	p->previous[2] = p->position[2];
	p->dampening = 0.35*drand48();
	p->alive = 1;
    }

    psTimeStep(p, 2*dt*drand48());
}



void
psBounce(PSparticle* p, float dt)
{
    float s;

    if (p->alive == 0)
	return;

    /* since we know it is the ground plane, we only need to
       calculate s for a single dimension. */
    s = -p->previous[1]/p->velocity[1];

    p->position[0] = (p->previous[0] + p->velocity[0] * s + 
		      p->velocity[0] * (dt-s) * p->dampening);
    p->position[1] = -p->velocity[1] * (dt-s) * p->dampening; /* reflect */
    p->position[2] = (p->previous[2] + p->velocity[2] * s + 
		      p->velocity[2] * (dt-s) * p->dampening);

    /* damp the reflected velocity (since the particle hit something,
       it lost some energy) */
    p->velocity[0] *=  p->dampening;
    p->velocity[1] *= -p->dampening;		/* reflect */
    p->velocity[2] *=  p->dampening;
}

void
psCollideSphere(PSparticle* p, PSsphere* s)
{
    float vx = p->position[0] - s->x;
    float vy = p->position[1] - s->y;
    float vz = p->position[2] - s->z;
    float distance;

    if (p->alive == 0)
	return;

    distance = sqrt(vx*vx + vy*vy + vz*vz);

    if (distance < s->radius) {
#if 0
	vx /= distance;  vy /= distance;  vz /= distance;
	d = 2*(-vx*p->velocity[0] + -vy*p->velocity[1] + -vz*p->velocity[2]);
	p->velocity[0] += vx*d*2;
	p->velocity[1] += vy*d*2;
	p->velocity[2] += vz*d*2;
	d = sqrt(p->velocity[0]*p->velocity[0] + 
		 p->velocity[1]*p->velocity[1] +
		 p->velocity[2]*p->velocity[2]);
	p->velocity[0] /= d;
	p->velocity[1] /= d;
	p->velocity[2] /= d;
#else
	p->position[0] = s->x+(vx/distance)*s->radius;
	p->position[1] = s->y+(vy/distance)*s->radius;
	p->position[2] = s->z+(vz/distance)*s->radius;
	p->previous[0] = p->position[0];
	p->previous[1] = p->position[1];
	p->previous[2] = p->position[2];
	p->velocity[0] = vx/distance;
	p->velocity[1] = vy/distance;
	p->velocity[2] = vz/distance;
#endif
    }
}


void
reshape(int width, int height)
{
    float black[] = { 0, 0, 0, 0 };

    glViewport(0, 0, width, height);/*glViewport specifies the affine transformation of x and y from normalized device coordinates to window coordinates*/
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, (float)width/height, 0.1, 1000);/*specifies a viewing frustum into the world coordinate system. In general, the aspect ratio in gluPerspective should match the aspect ratio of the associated viewport.*/
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 1, 3, 0, 1, 0, 0, 1, 0);/*gluLookAt creates a viewing matrix derived from an eye point, a reference point indicating the center of the scene, and an UP vector.*/
    glFogfv(GL_FOG_COLOR, black);/*Fog is initially disabled. While enabled, fog affects rasterized geometry, bitmaps, and pixel blocks, but not buffer clear operations. To enable and disable fog, call glEnable and glDisable with argument GL_FOG.*/
    glFogf(GL_FOG_START, 2.5);
    glFogf(GL_FOG_END, 4);
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glPointSize(point_size);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);/*glBlendFunc defines the operation of blending when it is enabled.*/
    glEnable(GL_COLOR_MATERIAL);/*specifies which material parameters track the current color*/
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHT0);
    timedelta();
}


void
display(void)
{
    static int i;
    static float c;
    static int j = 0;
    static char s[32];
    static int frames = 0;

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);/*Indicates the buffers currently enabled for color writing.Indicates the depth buffer*/
    glPushMatrix();

    glRotatef(spin_y, 1, 0, 0); /*produces a rotation of angle degrees around the vector x y z */
    glRotatef(spin_x, 0, 1, 0);

    glEnable(GL_LIGHTING);/*If it's disabled then all polygons, lines and points will be coloured according to the setting of the various forms of the glColor command. */
    if (draw_spheres) {
	for (i = 0; i < draw_spheres; i++) {
	    glPushMatrix();
	    glTranslatef(spheres[i].x, spheres[i].y, spheres[i].z);/*multiply the current matrix by a translation matrix*/
	    glColor3f(0.0, 1.0, 0.0);/*color for sphere*/
	    glutSolidSphere(spheres[i].radius, 20,20);
	    glPopMatrix();
	}
    }
    glDisable(GL_LIGHTING);

    glBegin(GL_QUADS);
    glColor3f(1.0, 0.0, 0.0);/*color for square block at the bottam */
    glVertex3f(-2, 0, -2);
    glVertex3f(-2, 0, 2);
    glVertex3f(2, 0, 2);
    glVertex3f(2, 0, -2);
    glEnd();

    if (points) {
	glBegin(GL_POINTS);
	
	for (i = 0; i < num_particles; i++) {
	    if (particles[i].alive == 0)
		continue;
	    c = particles[i].position[1]/2.1*255;
	    glColor3ub(c, 128+c*0.6, 175);/* color of fountain particles*/
	    glVertex3fv(particles[i].position);
	}
	glEnd();
    } else {
	glBegin(GL_LINES);
	for (i = 0; i < num_particles; i++) {
	    if (particles[i].alive == 0)
		continue;
	    c = particles[i].previous[1]/2.1*255;
	    glColor4ub(c, 128+c*0.7, 255, 32);
	    glVertex3fv(particles[i].previous);
	    c = particles[i].position[1]/2.1*255;
	    glColor4ub(c, 128+c*0.3, 255, 196);
	    glVertex3fv(particles[i].position);
	}
	glEnd();
    }
    sprintf(s, "PROJECT BY SREERAG & SOURJITH" );
	text(0, 660, s);
    
	

    glPopMatrix();
    glutSwapBuffers();/*Performs a buffer swap on the layer in use for the current window*/
}

void
idle(void)
{
    static int i, j;
    static int living = 0;		/* index to end of live particles */
    static float dt;
    static float last = 0;

    dt = timedelta();
    frame_time += dt;

#if 1
    /* slow the simulation if we can't keep the frame rate up around
       10 fps */
    if (dt > 0.1) {
	slow_down = 1.0/(100*dt);
    } else if (dt < 0.1) {
	slow_down = 1;
    }
#endif

    dt *= slow_down;

    /* resurrect a few particles */
    for (i = 0; i < flow*dt; i++) {
	psNewParticle(&particles[living], dt);
	living++;
	if (living >= num_particles)
	    living = 0;
    }

    for (i = 0; i < num_particles; i++) {
	psTimeStep(&particles[i], dt);

	/* collision with sphere? */
	if (draw_spheres) {
	    for (j = 0; j < draw_spheres; j++) {
		psCollideSphere(&particles[i], &spheres[j]);
	    }
	}

	/* collision with ground? */
	if (particles[i].position[1] <= 0) {
	    psBounce(&particles[i], dt);
	}

	/* dead particle? */
	if (particles[i].position[1] < 0.1 && 
	    fequal(particles[i].velocity[1], 0)) {
	    particles[i].alive = 0;
	}
    }

    glutPostRedisplay();
}

void
visible(int state)
{
    if (state == GLUT_VISIBLE)
	glutIdleFunc(idle);
    else
	glutIdleFunc(NULL);
}

void
bail(int code)
{
    free(particles);
    exit(code);
}

#ifdef SCREEN_SAVER_MODE
void
ss_keyboard(char key, int x, int y)
{
    bail(0);
}

void
ss_mouse(int button, int state, int x, int y)
{
    bail(0);
}

void
ss_passive(int x, int y)
{
    static int been_here = 0;

  
    if (been_here > 1)
	bail(0);
    been_here++;
}

#else

void
keyboard(unsigned char key, int x, int y)
{
    static int fullscreen = 0;
    static int old_x = 50;
    static int old_y = 50;
    static int old_width = 512;
    static int old_height = 512;
    static int s = 0;

	if(key=='w'||key=='W')
		glClearColor(0.0,1.0,0.0,1.0);
	if(key=='p'||key=='P')
		glClearColor(0.0,0.0,1.0,1.0);
	if(key=='e'||key=='E')
		glClearColor(0.5,0.0,1.0,1.0);
	if(key=='r'||key=='R')
		glClearColor(0.4,1.0,0.5,1.0);
	if(key=='t'||key=='T')
		glClearColor(0.8,0.9,0.6,1.0);
	if(key=='y'||key=='Y')
		glClearColor(0.5,0.4,0.0,1.0);
	if(key=='u'||key=='U')
		glClearColor(1.0,1.0,0.0,1.0);
	if(key=='i'||key=='I')
		glClearColor(1.0,1.4,2.0,1.0);
	if(key=='o'||key=='O')
		glClearColor(0.0,0.0,0.0,1.0);
	if(key=='q'||key=='Q')
		glClearColor(1.0,0.0,0.0,1.0);

    switch (key) {
    case 27:
        bail(0);
	break;

    case 'f':
	if (type == PS_WATERFALL)
	    type = PS_FOUNTAIN;
	else if (type == PS_FOUNTAIN)
	    type = PS_WATERFALL;
	break;

    case 's':
	draw_spheres++;
	if (draw_spheres > NUM_SPHERES)
	    draw_spheres = 0;
	break;

    case 'S':
	printf("PSsphere spheres[NUM_SPHERES] = {/* position of spheres */\n");
	for (s = 0; s < NUM_SPHERES; s++) {
	    printf("  { %g, %g, %g, %g },\n", 
		   spheres[s].x, spheres[s].y,
		   spheres[s].z, spheres[s].radius);
	}
	printf("};\n");
	break;

    case 'l':
	points = !points;
	break;

    case 'c':
	point_size++;
	glPointSize(point_size);/*specifies the rasterized diameter of both aliased and antialiased points*/
	break;
	
    case 'k':
	point_size--;
	if (point_size < 1)
	    point_size = 1;
	glPointSize(point_size);
	break;
	
    case '+':
	flow += 100;
	if (flow > num_particles)
	    flow = num_particles;
	printf("%g particles/second\n", flow);
	break;

    case '-':
	flow -= 100;
	if (flow < 0)
	    flow = 0;
	printf("%g particles/second\n", flow);
	break;

    case '#':
	frame_rate = !frame_rate;
	break;

    case '~':
	fullscreen = !fullscreen;
	if (fullscreen) {
	    old_x = glutGet(GLUT_WINDOW_X);
	    old_y = glutGet(GLUT_WINDOW_Y);
	    old_width = glutGet(GLUT_WINDOW_WIDTH);
	    old_height = glutGet(GLUT_WINDOW_HEIGHT);
	    glutFullScreen();
	} else {
	    glutReshapeWindow(old_width, old_height);
	    glutPositionWindow(old_x, old_y);
	}
	break;

    case 'z':
	s++;
	if (s >= NUM_SPHERES)
	    s = 0;
	break;

    case '4':
	spheres[s].x -= 0.05;
	break;

    case '6':
	spheres[s].x += 0.05;
	break;

    case '2':
	spheres[s].y -= 0.05;
	break;

    case '8':
	spheres[s].y += 0.05;
	break;

    case '7':
	spheres[s].z -= 0.05;
	break;

    case '3':
	spheres[s].z += 0.05;
	break;

    case '9':
	spheres[s].radius += 0.05;
	break;

    case '1':
	spheres[s].radius -= 0.05;
	break;

    }
}

void
menu(int item)
{
    keyboard((unsigned char)item, 0, 0);
}

void
menustate(int state)
{
    /* hook up a fake time delta to avoid jumping when menu comes up */
    if (state == GLUT_MENU_NOT_IN_USE)
	timedelta();
}
#endif

int old_x, old_y;

void
mouse(int button, int state, int x, int y)
{
    old_x = x;
    old_y = y;

    glutPostRedisplay();
}

void
motion(int x, int y)
{
    spin_x = x - old_x;
    spin_y = y - old_y;

    glutPostRedisplay();
}

int
main(int argc, char** argv)
{
    glutInitDisplayMode(GLUT_RGB|GLUT_DEPTH|GLUT_DOUBLE);/*sets the initial display mode .Bit mask to select an RGBA mode window.Bit mask to select a double buffered window.Bit mask to select a window with a depth buffer*/
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(640, 480);
    glutInit(&argc, argv);/*Initializes GLUT and should be called before any OpenGL functions: glutInit( ) takes the arguement from main( ) and can use them in an implmementation-dependent manner.*/

    glutCreateWindow("fountain effect");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);/*sets the reshape callback for the current window. The reshape callback is triggered when a window is reshaped. */
#ifdef SCREEN_SAVER_MODE
    glutPassiveMotionFunc(ss_passive);
    glutKeyboardFunc(ss_keyboard);
    glutMouseFunc(ss_mouse);
    glutSetCursor(GLUT_CURSOR_NONE);
    glutFullScreen(); 
#else
    glutMotionFunc(motion);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
#endif

    glutMenuStateFunc(menustate);
    glutCreateMenu(menu);
    glutAddMenuEntry("Particle", 0);
    glutAddMenuEntry("", 0);
    glutAddMenuEntry("[t]   fountain type", 'f');
    glutAddMenuEntry("[s]   Collision spheres", 's');
    glutAddMenuEntry("[-]   Less flow", '-');
    glutAddMenuEntry("[+]   More flow", '+');
    glutAddMenuEntry("[k]   Smaller points", 'k');
    glutAddMenuEntry("[c]   Larger points", 'c');
    glutAddMenuEntry("[l]   Toggle points/lines", 'l');
    glutAddMenuEntry("[#]   Toggle name on/off", '#');
    glutAddMenuEntry("[~]   Toggle fullscreen on/off", '~');
    glutAddMenuEntry("", 0);
    glutAddMenuEntry("Use the numeric keypad to move the spheres", 0);
    glutAddMenuEntry("[z]   Change active sphere", 0);
    glutAddMenuEntry("", 0);
    glutAddMenuEntry("[Esc] Quit", 27);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    if (argc > 1) {
	if (strcmp(argv[1], "-h") == 0) {
	    fprintf(stderr, "%s [particles] [flow] [speed%]\n", argv[0]);
	    exit(0);
	}
	sscanf(argv[1], "%d", &num_particles);
	if (argc > 2)
	    sscanf(argv[2], "%f", &flow);
	if (argc > 3)
	    sscanf(argv[3], "%f", &slow_down);
    }      

    particles = (PSparticle*)malloc(sizeof(PSparticle) * num_particles);

    glutVisibilityFunc(visible);/*sets the visibility callback for the current window. The visibility callback for a window is called when the visibility of a window changes.*/
    glutMainLoop();/*enters the GLUT event processing loop.*/
    return 0;
}
