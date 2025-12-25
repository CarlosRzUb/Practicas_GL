#include <iostream>
#include "codebase.h"

using namespace std;
using namespace cb;

#define PROYECTO "P9 - Simulador de Vuelo"

bool fullscreen = false;

const int DIM_ESPACIO = 50;
const int MAX_SUB_ROCAS = 50;

Sistema3d camara(Vec3(1, 0, 0), Vec3(0, 0, 1), Vec3(0, -1, 0), Vec3(0, 0, 2));

float velocidad = 0.0f;
const float VEL_MAX = 20.0f;
const float VEL_INC = 10.0f;
const float VEL_GIRO_METEORITO = 15.0f;
const float SENSIBILIDAD = 0.002f;

int x_ant = 0, y_ant = 0;
bool raton_iniciado = false;
int t_anterior = 0;

bool luces_on = true;

const GLfloat LUZ_SOL_DIFUSA[] = { 1.0f, 1.0f, 0.9f, 1.0f };
const GLfloat LUZ_SOL_AMBIENTE[] = { 0.1f, 0.1f, 0.1f, 1.0f };
const GLfloat LUZ_FOCO_DIFUSA[] = { 0.8f, 0.8f, 1.0f, 1.0f };
const GLfloat LUZ_FOCO_ESPECULAR[] = { 1.0f, 1.0f, 1.0f, 1.0f };

const GLfloat MAT_AMBIENTE[] = { 0.2f, 0.15f, 0.1f, 1.0f };
const GLfloat MAT_DIFUSO[] = { 0.6f, 0.4f, 0.2f, 1.0f };
const GLfloat MAT_ESPECULAR[] = { 0.5f, 0.5f, 0.5f, 1.0f };
const GLfloat MAT_BRILLO[] = { 20.0f };

const int NUM_METEORITOS = 10;

GLuint texID_suelo;
GLuint texID_meteorito;
GLuint texID_cielo;
GLuint texID_nave;

bool cabina_on = true;

struct SubRoca {
    Vec3 offsetPos;
    Vec3 offsetGiro;
    float escalaLocal;
    GLfloat vertices[8][3];
};

struct Meteorito {
    Vec3 pos;
    Vec3 ejeGiro;
    float angulo;
    float velocidadGiro;
    float escalaGlobal;
    int numSubRocas;
    SubRoca partes[MAX_SUB_ROCAS];
};

Meteorito meteoritos[NUM_METEORITOS];

void setObserver() {
    Vec3 e = camara.geto();
    Vec3 u = camara.getu();
    Vec3 v = camara.getv();
    Vec3 w = camara.getw();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(e.x, e.y, e.z,
        e.x - w.x, e.y - w.y, e.z - w.z,
        v.x, v.y, v.z);
}

void setFocosNave() {
    if (luces_on) {
        glEnable(GL_LIGHT1);
        glEnable(GL_LIGHT2);

        GLfloat pos_izq[] = { -1.5f, 0.0f, 0.0f, 1.0f };
        GLfloat pos_der[] = { 1.5f, 0.0f, 0.0f, 1.0f };
        GLfloat direccion[] = { 0.0f, 0.0f, -1.0f };

        glLightfv(GL_LIGHT1, GL_POSITION, pos_izq);
        glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, direccion);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, LUZ_FOCO_DIFUSA);
        glLightfv(GL_LIGHT1, GL_SPECULAR, LUZ_FOCO_ESPECULAR);
        glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 30.0f);
        glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 10.0f);

        glLightfv(GL_LIGHT2, GL_POSITION, pos_der);
        glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, direccion);
        glLightfv(GL_LIGHT2, GL_DIFFUSE, LUZ_FOCO_DIFUSA);
        glLightfv(GL_LIGHT2, GL_SPECULAR, LUZ_FOCO_ESPECULAR);
        glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 30.0f);
        glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 10.0f);
    }
    else {
        glDisable(GL_LIGHT1);
        glDisable(GL_LIGHT2);
    }
}

void setSol() {
    glEnable(GL_LIGHT0);
    GLfloat pos_sol[] = { 1.0f, 1.0f, 1.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, pos_sol);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, LUZ_SOL_DIFUSA);
    glLightfv(GL_LIGHT0, GL_AMBIENT, LUZ_SOL_AMBIENTE);
}

void drawRoca(GLfloat v[8][3]) {
    quadtex(v[0], v[1], v[2], v[3], 0, 1, 0, 1, 1, 1);
    quadtex(v[5], v[4], v[7], v[6], 0, 1, 0, 1, 1, 1);
    quadtex(v[1], v[5], v[6], v[2], 0, 1, 0, 1, 1, 1);
    quadtex(v[4], v[0], v[3], v[7], 0, 1, 0, 1, 1, 1);
    quadtex(v[3], v[2], v[6], v[7], 0, 1, 0, 1, 1, 1);
    quadtex(v[4], v[5], v[1], v[0], 0, 1, 0, 1, 1, 1);
}

void drawMeteoritos() {
    GLfloat mat_blanco[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_blanco);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID_meteorito);

    for (int i = 0; i < NUM_METEORITOS; i++) {
        glPushMatrix();
        glTranslatef(meteoritos[i].pos.x, meteoritos[i].pos.y, meteoritos[i].pos.z);
        glRotatef(meteoritos[i].angulo, meteoritos[i].ejeGiro.x, meteoritos[i].ejeGiro.y, meteoritos[i].ejeGiro.z);
        glScalef(meteoritos[i].escalaGlobal, meteoritos[i].escalaGlobal, meteoritos[i].escalaGlobal);

        for (int j = 0; j < meteoritos[i].numSubRocas; j++) {
            glPushMatrix();
            glTranslatef(meteoritos[i].partes[j].offsetPos.x, meteoritos[i].partes[j].offsetPos.y, meteoritos[i].partes[j].offsetPos.z);
            glRotatef(meteoritos[i].partes[j].offsetGiro.x, 1, 0, 0);
            glRotatef(meteoritos[i].partes[j].offsetGiro.y, 0, 1, 0);
            glScalef(meteoritos[i].partes[j].escalaLocal, meteoritos[i].partes[j].escalaLocal, meteoritos[i].partes[j].escalaLocal);

            drawRoca(meteoritos[i].partes[j].vertices);

            glPopMatrix();
        }

        glPopMatrix();
    }

    glDisable(GL_TEXTURE_2D);
}

void renderString(float x, float y, void* font, const char* string) {
    glRasterPos2f(x, y);
    const char* c;
    for (c = string; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
}

void drawScene() {
    glPushMatrix();

    glMaterialfv(GL_FRONT, GL_AMBIENT, MAT_AMBIENTE);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, MAT_DIFUSO);
    glMaterialfv(GL_FRONT, GL_SPECULAR, MAT_ESPECULAR);
    glMaterialfv(GL_FRONT, GL_SHININESS, MAT_BRILLO);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID_suelo);
    glColor3f(1.0f, 1.0f, 1.0f);

    glScalef((float)DIM_ESPACIO, (float)DIM_ESPACIO, 1.0f);

    planoXY(50);

    glDisable(GL_TEXTURE_2D);
    glPopMatrix();

    drawMeteoritos();

    glDisable(GL_LIGHTING);
    ejes();
    glEnable(GL_LIGHTING);
}

void drawSkybox() {
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID_cielo);
    glColor3f(1.0f, 1.0f, 1.0f);

    GLUquadricObj* q = gluNewQuadric();

    gluQuadricTexture(q, GL_TRUE);

    gluQuadricOrientation(q, GLU_INSIDE);

    glPushMatrix();
    glRotatef(-90, 1, 0, 0);

    gluSphere(q, 800.0f, 64, 64);

    glPopMatrix();

    gluDeleteQuadric(q);

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_FOG);
}

void drawCabina() {
    if (!cabina_on) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    gluOrtho2D(0.0, 1.0, 0.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID_nave);
    glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 1.0f);
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
    glColor3f(0.0f, 1.0f, 0.0f);

    int linea = 20;
    renderString(10, h - linea, GLUT_BITMAP_HELVETICA_12, "CONTROLES DE VUELO:"); // Título más grande
    linea += 20;
    renderString(10, h - linea, GLUT_BITMAP_HELVETICA_12, "[Raton] Orientacion");
    linea += 20;
    renderString(10, h - linea, GLUT_BITMAP_HELVETICA_12, "[A]     Acelerar");
    linea += 20;
    renderString(10, h - linea, GLUT_BITMAP_HELVETICA_12, "[Z]     Frenar");
    linea += 20;
    renderString(10, h - linea, GLUT_BITMAP_HELVETICA_12, "[L]     Luces");
    linea += 20;
    renderString(10, h - linea, GLUT_BITMAP_HELVETICA_12, "[C]     Cabina");
    linea += 20;
    renderString(10, h - linea, GLUT_BITMAP_HELVETICA_12, "[F]     Pantalla Completa");
    linea += 20;
    renderString(10, h - linea, GLUT_BITMAP_HELVETICA_12, "[ESC]   Salir");

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    setFocosNave();
    setObserver();
    setSol();

    drawSkybox();
    drawScene();
    drawCabina();

    glutSwapBuffers();
}

void resize(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w / h, 0.1, 1500.0);
}

void onKey(unsigned char key, int x, int y) {
    switch (key) {
    case 'a': case 'A':
        velocidad += VEL_INC;
        if (velocidad > VEL_MAX) velocidad = VEL_MAX;
        break;
    case 'z': case 'Z':
        velocidad -= VEL_INC;
        if (velocidad < 0.0f) velocidad = 0.0f;
        break;
    case 'l': case 'L':
        luces_on = !luces_on;
        break;
    case 'c': case 'C':
        cabina_on = !cabina_on;
        break;
    case 'f': case 'F':
        fullscreen = !fullscreen;
        if (fullscreen) {
            glutFullScreen();
        }
        else {
            glutReshapeWindow(800, 600);
            glutPositionWindow(100, 100);
        }
        break;
    case 27:
        exit(0);
        break;
    }
    glutPostRedisplay();
}

void onPassiveMotion(int x, int y) {
    int centrox = glutGet(GLUT_WINDOW_WIDTH) / 2;
    int centroy = glutGet(GLUT_WINDOW_HEIGHT) / 2;

    if (x == centrox && y == centroy) return;

    int dx = x - centrox;
    int dy = y - centroy;

    if (dx != 0) {
        camara.rotar(-dx * SENSIBILIDAD, Vec3(0, 0, 1));
    }

    if (dy != 0) {
        camara.rotar(-dy * SENSIBILIDAD, camara.getu());
    }

    glutWarpPointer(centrox, centroy);

    glutPostRedisplay();
}

void onIdle() {
    int t_actual = glutGet(GLUT_ELAPSED_TIME);
    float dt = (t_actual - t_anterior) / 1000.0f;
    t_anterior = t_actual;

    if (dt > 0.1f) dt = 0.1f;

    if (abs(velocidad) > 0.001f) {
        Vec3 direccion = camara.getw() * (-velocidad * dt);
        camara.seto(camara.geto() + direccion);
        glutPostRedisplay();
    }

    for (int i = 0; i < NUM_METEORITOS; i++) {
        meteoritos[i].angulo += (meteoritos[i].velocidadGiro * VEL_GIRO_METEORITO) * dt;
        if (meteoritos[i].angulo > 360.0f) meteoritos[i].angulo -= 360.0f;
    }

    glutPostRedisplay();
}

void initMeteoritos() {
    srand(1234);

    float base[8][3] = {
        {-0.5, -0.5,  0.5}, { 0.5, -0.5,  0.5}, { 0.5,  0.5,  0.5}, {-0.5,  0.5,  0.5},
        {-0.5, -0.5, -0.5}, { 0.5, -0.5, -0.5}, { 0.5,  0.5, -0.5}, {-0.5,  0.5, -0.5}
    };

    float RANGO = 300.0f;

    for (int i = 0; i < NUM_METEORITOS; i++) {
        float rX = (rand() % (int)RANGO) - (RANGO / 2.0f);
        float rY = (rand() % (int)RANGO) - (RANGO / 2.0f);
        float rZ = (rand() % (int)RANGO) - (RANGO / 2.0f);

        meteoritos[i].pos = Vec3(rX, rY, rZ);
        meteoritos[i].ejeGiro = Vec3((rand() % 100) / 100.0f, (rand() % 100) / 100.0f, (rand() % 100) / 100.0f);
        meteoritos[i].angulo = 0.0f;
        meteoritos[i].velocidadGiro = 0.1f + ((rand() % 100) / 200.0f);

        meteoritos[i].escalaGlobal = 4.0f + ((rand() % 40) / 10.0f);

        meteoritos[i].numSubRocas = 5 + (rand() % (MAX_SUB_ROCAS - 5));

        for (int j = 0; j < meteoritos[i].numSubRocas; j++) {
            meteoritos[i].partes[j].offsetPos = Vec3(
                ((rand() % 100) / 100.0f) - 0.5f,
                ((rand() % 100) / 100.0f) - 0.5f,
                ((rand() % 100) / 100.0f) - 0.5f
            );
            meteoritos[i].partes[j].offsetGiro = Vec3(rand() % 360, rand() % 360, rand() % 360);
            meteoritos[i].partes[j].escalaLocal = 0.3f + ((rand() % 100) / 150.0f);

            for (int v = 0; v < 8; v++) {
                float deform = 0.2f;
                meteoritos[i].partes[j].vertices[v][0] = base[v][0] + (((rand() % 100) / 250.0f) - deform);
                meteoritos[i].partes[j].vertices[v][1] = base[v][1] + (((rand() % 100) / 250.0f) - deform);
                meteoritos[i].partes[j].vertices[v][2] = base[v][2] + (((rand() % 100) / 250.0f) - deform);
            }
        }
    }
}

void initTexturas() {
    glGenTextures(1, &texID_suelo);
    glBindTexture(GL_TEXTURE_2D, texID_suelo);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    loadImageFile((char*)"metal.jpg");

    glGenTextures(1, &texID_meteorito);
    glBindTexture(GL_TEXTURE_2D, texID_meteorito);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    loadImageFile((char*)"roca.jpg");

    glGenTextures(1, &texID_cielo);
    glBindTexture(GL_TEXTURE_2D, texID_cielo);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    loadImageFile((char*)"estrellas.jpg");

    glGenTextures(1, &texID_nave);
    glBindTexture(GL_TEXTURE_2D, texID_nave);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    loadImageFile((char*)"nave.png");
}

void init() {
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_FOG);
    GLfloat colorNiebla[] = { 0.05f, 0.05f, 0.1f, 1.0f };
    glFogfv(GL_FOG_COLOR, colorNiebla);

    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, 10.0f);
    glFogf(GL_FOG_END, 350.0f);

    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);

    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer(800 / 2, 600 / 2);

    initMeteoritos();
    initTexturas();

    t_anterior = glutGet(GLUT_ELAPSED_TIME);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow(PROYECTO);

    init();

    cout << "--- CONTROLES ---" << endl;
    cout << " [Ratón] Mover: Girar la nave (Vista infinita)" << endl;
    cout << " [A]     Tecla: Acelerar" << endl;
    cout << " [Z]     Tecla: Frenar" << endl;
    cout << " [L]     Tecla: Luces On/Off" << endl;
    cout << " [C]     Tecla: Cabina On/Off" << endl;
    cout << " [F]     Tecla: Pantalla Completa On/Off" << endl;
    cout << " [ESC]   Salir" << endl;

    glutDisplayFunc(display);
    glutReshapeFunc(resize);
    glutKeyboardFunc(onKey);
    glutPassiveMotionFunc(onPassiveMotion);
    glutIdleFunc(onIdle);

    glutMainLoop();
    return 0;
}
