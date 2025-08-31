#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <iostream>

using namespace std;

int W=1280,H=800;
float T=0.f;


struct Camera {
    float x, y, z;          
    float pitch, yaw;       
    float speed;            
    bool freeMode;          
} camera = {30.0f, 5.0f, 0.0f, 0.0f, 0.0f, 2.0f, false};


int lastMouseX = W/2, lastMouseY = H/2;
bool mousePressed = false;


bool showFPS = true;
int frameCount = 0;
float lastTime = 0.0f;
float currentFPS = 0.0f;

struct Particle{float a,z,r,spd,band,jx,jy;};
vector<Particle> pts,stars;

float now(){ static auto t0=chrono::high_resolution_clock::now(); auto t=chrono::high_resolution_clock::now(); return chrono::duration<float>(t-t0).count(); }

void gen(int n=24000){
    pts.resize(n);
    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> U(0.f,1.f),S(-1.f,1.f);
    for(auto &p:pts){
        p.a=6.2831853f*U(rng);
        p.z=-1200.f*U(rng)-40.f;
        p.r=4.f+26.f*powf(U(rng),0.7f);
        p.spd=18.f+48.f*U(rng);
        p.band=floorf(U(rng)*7.f);
        p.jx=0.6f*S(rng);
        p.jy=0.6f*S(rng);
    }
    int m=5000;
    stars.resize(m);
    for(int i=0;i<m;i++){
        float a=6.2831853f*U(rng);
        float R=140.f*sqrtf(U(rng));
        stars[i]={a,-4000.f*U(rng)-200.f,R*cosf(a),10.f+R*sinf(a),14.f+20.f*U(rng),0.f,0.f};
    }
}

void proj(){
    glViewport(0,0,W,H);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(72.0,(double)W/(double)H,0.1,6000.0);
    glMatrixMode(GL_MODELVIEW);
}

void camera_control(){
    if (!camera.freeMode) {
        
        float r=28.f+7.f*sinf(0.32f*T);
        float a=0.3f*T;
        float h=3.8f+2.8f*sinf(0.21f*T+0.9f);
        gluLookAt(r*cosf(a),h,r*sinf(a),0,0,-260,0,1,0);
    } else {
        
        glLoadIdentity();
        
        
        glRotatef(camera.pitch, 1.0f, 0.0f, 0.0f);
        glRotatef(camera.yaw, 0.0f, 1.0f, 0.0f);
        
        
        glTranslatef(-camera.x, -camera.y, -camera.z);
    }
}

void drawStars(){
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE,GL_ONE);
    glPointSize(1.8f);
    glBegin(GL_POINTS);
    for(auto &s:stars){
        float tw=0.65f+0.35f*sinf(0.6f*T+s.a*3.f);
        glColor4f(0.45f*tw,0.55f*tw,1.0f*tw,1.0f);
        glVertex3f(s.r,s.spd,s.z);
    }
    glEnd();
    glEnable(GL_DEPTH_TEST);
}

void colorBH(float u,float v,float &r,float &g,float &b){
    float c1=0.5f+0.5f*sinf(6.2831853f*(u+0.05f*T));
    float c2=0.5f+0.5f*sinf(6.2831853f*(u*0.5f+0.3f*v)+2.1f+0.1f*T);
    float c3=0.5f+0.5f*sinf(6.2831853f*(u*0.9f-0.2f*v)+3.6f-0.07f*T);
    float blue = 0.55f+0.45f*c1;
    float purple = 0.45f+0.55f*c2;
    float pink = 0.55f+0.45f*c3;
    r = 0.25f*blue + 0.35f*purple + 0.80f*pink;
    g = 0.35f*blue + 0.45f*purple + 0.30f*pink;
    b = 1.00f*blue + 0.60f*purple + 0.20f*pink;
}

void pass(float ps,float alphaMul,float znear,float zfar,float swirl,float kdepth){
    const float INNER_R = 10.0f;
    glPointSize(ps);
    glBegin(GL_POINTS);
    for(auto &p:pts){
        float z=p.z+fmodf(T*p.spd,1400.f);
        if(z>znear||z<zfar) continue;
        float a=p.a + swirl*T + 0.0019f*z + p.band*(6.2831853f/7.f);
        float r=p.r*(1.f+0.0011f*z);
        if(r<INNER_R) r=INNER_R;
        float wob=0.8f*sinf(0.7f*T+p.band*0.8f+0.02f*z);
        float x=(r+wob)*cosf(a)+p.jx*sinf(0.9f*T+0.01f*z);
        float y=(r-wob)*sinf(a)+p.jy*cosf(0.8f*T+0.013f*z);
        float u=fmodf(0.0025f*z + 0.12f*p.band,1.f);
        float v=fabsf(z)/1400.f;
        float cr,cg,cb; colorBH(u,v,cr,cg,cb);
        float glow=0.6f+0.4f*sinf(2.4f*T+0.3f*p.band+0.003f*z);
        float centerFade = 0.6f + 0.4f*(r/INNER_R);
        float fade=(1.f - fminf(1.f,v))*alphaMul*glow*centerFade;
        glColor4f(cr,cg,cb,fade);
        glVertex3f(x,y,z);
    }
    glEnd();
}

void drawFPS() {
    if (!showFPS) return;
    
    
    frameCount++;
    float currentTime = now();
    if (currentTime - lastTime >= 1.0f) {
        currentFPS = frameCount / (currentTime - lastTime);
        frameCount = 0;
        lastTime = currentTime;
    }
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, W, 0, H, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glDisable(GL_DEPTH_TEST);
    glColor3f(0.0f, 1.0f, 0.0f);
    
    char fpsStr[100];
    sprintf(fpsStr, "FPS: %.1f | Camera: %s | [C] Modo Camara | [F] Modo FPS", 
            currentFPS, camera.freeMode ? "LIBRE" : "AUTO");
    
    glRasterPos2f(10, H - 25);
    for (char* c = fpsStr; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
    
    if (camera.freeMode) {
        char controlStr[] = "WASD: Movimiento | QE: Arriba/Abajo";
        glRasterPos2f(10, H - 45);
        for (char* c = controlStr; *c; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }
    }
    
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void draw(){
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE,GL_ONE);
    glDisable(GL_LIGHTING);
    camera_control();
    drawStars();
    pass(1.8f,0.25f,900.f,-2600.f,0.35f,0.0019f);
    pass(2.6f,0.5f,280.f,-1200.f,0.45f,0.0021f);
    pass(4.2f,0.95f,80.f,-520.f,0.55f,0.0023f);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    
    drawFPS();
}

void display(){
    T=now();
    glClearColor(0.02f,0.02f,0.06f,1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    draw();
    glutSwapBuffers();
}

void reshape(int w,int h){ W=w; H=h; proj(); }
void idle(){ glutPostRedisplay(); }

void key(unsigned char k, int x, int y) {
    float moveSpeed = camera.speed;
    
    switch(k) {
        case 27: exit(0); break; 
        case 'c': case 'C': 
            camera.freeMode = !camera.freeMode;
            if (!camera.freeMode) {
                
                camera.x = 30.0f; camera.y = 5.0f; camera.z = 0.0f;
                camera.pitch = 0.0f; camera.yaw = 0.0f;
            }
            break;
        case 'w': case 'W':
            if (camera.freeMode) {
                camera.x += moveSpeed * sinf(camera.yaw * M_PI/180.0f);
                camera.z -= moveSpeed * cosf(camera.yaw * M_PI/180.0f);
            }
            break;
        case 's': case 'S':
            if (camera.freeMode) {
                camera.x -= moveSpeed * sinf(camera.yaw * M_PI/180.0f);
                camera.z += moveSpeed * cosf(camera.yaw * M_PI/180.0f);
            }
            break;
        case 'a': case 'A':
            if (camera.freeMode) {
                camera.x -= moveSpeed * cosf(camera.yaw * M_PI/180.0f);
                camera.z -= moveSpeed * sinf(camera.yaw * M_PI/180.0f);
            }
            break;
        case 'd': case 'D':
            if (camera.freeMode) {
                camera.x += moveSpeed * cosf(camera.yaw * M_PI/180.0f);
                camera.z += moveSpeed * sinf(camera.yaw * M_PI/180.0f);
            }
            break;
        case 'q': case 'Q':
            if (camera.freeMode) camera.y += moveSpeed;
            break;
        case 'e': case 'E':
            if (camera.freeMode) camera.y -= moveSpeed;
            break;
        case 'f': case 'F':
            showFPS = !showFPS;
            break;
    }
}


void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        mousePressed = (state == GLUT_DOWN);
        lastMouseX = x;
        lastMouseY = y;
    }
}

void motion(int x, int y) {
    if (camera.freeMode && mousePressed) {
        float deltaX = x - lastMouseX;
        float deltaY = y - lastMouseY;
        
        camera.yaw += deltaX * 0.2f;
        camera.pitch += deltaY * 0.2f;
        
        
        if (camera.pitch > 89.0f) camera.pitch = 89.0f;
        if (camera.pitch < -89.0f) camera.pitch = -89.0f;
        
        lastMouseX = x;
        lastMouseY = y;
    }
}

int main(int argc,char**argv){    
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(W,H);
    glutCreateWindow("Agujero de gusano");
    proj(); gen();
    glEnable(GL_POINT_SMOOTH);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutMainLoop();
    return 0;
}