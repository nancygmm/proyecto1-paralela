#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <omp.h>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <iostream>
#include <fstream>

using namespace std;

int W=1280,H=800;
float T=0.f;


int PARTICLE_COUNT = 200000;      
int STAR_COUNT = 15000;           
int MATH_ITERATIONS = 15;         
bool HEAVY_MATH_MODE = true;      
int num_threads = 4;              


chrono::high_resolution_clock::time_point start_time;
chrono::high_resolution_clock::time_point frame_start;
chrono::high_resolution_clock::time_point frame_end;
double total_gen_time = 0.0;
double total_draw_time = 0.0;
double total_computation_time = 0.0;
double total_parallel_time = 0.0;
int frame_count_timing = 0;
bool timing_enabled = true;


struct RenderData {
    float x, y, z;
    float r, g, b, a;
    bool visible;
};

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


vector<RenderData> particle_render_data;
vector<RenderData> star_render_data;

float now(){ 
    static auto t0=chrono::high_resolution_clock::now(); 
    auto t=chrono::high_resolution_clock::now(); 
    return chrono::duration<float>(t-t0).count(); 
}


void saveTimingMetrics() {
    if (frame_count_timing > 0) {
        ofstream file("parallel_timing_results.txt", ios::app);
        file << "=== MÉTRICAS PARALELAS (OpenMP - ALTA CARGA) ===" << endl;
        file << "Número de hilos utilizados: " << num_threads << endl;
        file << "Partículas procesadas: " << PARTICLE_COUNT << endl;
        file << "Iteraciones matemáticas por partícula: " << MATH_ITERATIONS << endl;
        file << "Estrellas: " << STAR_COUNT << endl;
        file << "Frames procesados: " << frame_count_timing << endl;
        file << "Tiempo de generación por frame: " << (total_gen_time / frame_count_timing) * 1000 << " ms" << endl;
        file << "Tiempo de renderizado por frame: " << (total_draw_time / frame_count_timing) * 1000 << " ms" << endl;
        file << "Tiempo de cálculos paralelos por frame: " << (total_parallel_time / frame_count_timing) * 1000 << " ms" << endl;
        file << "Tiempo total de computación: " << total_computation_time << " segundos" << endl;
        file << "Tiempo por frame: " << (total_computation_time / frame_count_timing) * 1000 << " ms" << endl;
        file << "FPS basado en cálculos: " << frame_count_timing / total_computation_time << endl;
        file << "Operaciones matemáticas estimadas por frame: " << (PARTICLE_COUNT * MATH_ITERATIONS * 10) << endl;
        file << "=====================================" << endl << endl;
        file.close();
        
        cout << "\n=== MÉTRICAS PARALELAS FINALES ===" << endl;
        cout << "Número de hilos: " << num_threads << endl;
        cout << "Partículas: " << PARTICLE_COUNT << " | Iteraciones: " << MATH_ITERATIONS << endl;
        cout << "Frames procesados: " << frame_count_timing << endl;
        cout << "Tiempo total de computación: " << total_computation_time << " segundos" << endl;
        cout << "Tiempo por frame: " << (total_computation_time / frame_count_timing) * 1000 << " ms" << endl;
        cout << "FPS: " << frame_count_timing / total_computation_time << endl;
        cout << "====================================" << endl;
    }
}

void gen(int n = -1){
    if(n == -1) n = PARTICLE_COUNT;
    
    auto gen_start = chrono::high_resolution_clock::now();
    
    cout << "Generando " << n << " partículas PARALELO" << endl;
    
    pts.resize(n);
    particle_render_data.resize(n * 6); 
    
    
    #pragma omp parallel num_threads(num_threads)
    {
        
        thread_local std::mt19937 rng(1337 + omp_get_thread_num());
        thread_local std::uniform_real_distribution<float> U(0.f,1.f);
        thread_local std::uniform_real_distribution<float> S(-1.f,1.f);
        
        
        #pragma omp for schedule(dynamic, 100)
        for(int i = 0; i < n; i++){
            
            pts[i].a=6.2831853f*U(rng);
            pts[i].z=-1200.f*U(rng)-40.f;
            pts[i].r=4.f+26.f*powf(U(rng),0.7f);
            pts[i].spd=18.f+48.f*U(rng);
            pts[i].band=floorf(U(rng)*7.f);
            pts[i].jx=0.6f*S(rng);
            pts[i].jy=0.6f*S(rng);
            
            
            if(HEAVY_MATH_MODE) {
                for(int iter = 0; iter < MATH_ITERATIONS; iter++) {
                    float complexity_factor = 1.0f + iter * 0.1f;
                    float dummy = 0;
                    
                    
                    dummy += sinf(pts[i].a * complexity_factor) * cosf(pts[i].z * complexity_factor);
                    dummy += tanf(pts[i].r * 0.01f + iter) * sinf(pts[i].spd * 0.001f);
                    dummy += sqrtf(fabsf(pts[i].r * complexity_factor + 1));
                    dummy += powf(fabsf(pts[i].spd), 1.2f + 0.05f * iter);
                    dummy += expf(-fabsf(pts[i].jx) * 0.1f) * logf(fabsf(pts[i].jy) + 1.0f);
                    dummy += atanf(pts[i].band + iter) * sinhf(pts[i].a * 0.1f);
                    
                    for(int j = 0; j < 3; j++) {
                        dummy += cosf(pts[i].a + j) * sinf(pts[i].z + j);
                    }
                    
                    pts[i].a += dummy * 0.0001f;
                    pts[i].jx += dummy * 0.00005f;
                }
            }
        }
        
        
        #pragma omp single
        {
            cout << "  Progreso: Generación paralela con " << num_threads << " hilos..." << endl;
        }
    }
    
    
    int m = STAR_COUNT;
    stars.resize(m);
    star_render_data.resize(m);
    
    #pragma omp parallel num_threads(num_threads)
    {
        thread_local std::mt19937 rng(2674 + omp_get_thread_num());
        thread_local std::uniform_real_distribution<float> U(0.f,1.f);
        
        #pragma omp for schedule(static)
        for(int i=0; i<m; i++){
            float a=6.2831853f*U(rng);
            float R=140.f*sqrtf(U(rng));
            stars[i]={a,-4000.f*U(rng)-200.f,R*cosf(a),10.f+R*sinf(a),14.f+20.f*U(rng),0.f,0.f};
            
            
            if(HEAVY_MATH_MODE) {
                for(int iter = 0; iter < 5; iter++) {
                    float dummy = sinf(a * iter) + cosf(R * iter) + tanf(stars[i].spd * 0.01f);
                    stars[i].a += dummy * 0.0001f;
                }
            }
        }
    }
    
    auto gen_end = chrono::high_resolution_clock::now();
    double gen_time = chrono::duration<double>(gen_end - gen_start).count();
    
    if (timing_enabled) {
        total_gen_time += gen_time;
    }
    
    cout << "Generación PARALELA completada en " << gen_time << " segundos" << endl;
    cout << "   • Hilos utilizados: " << num_threads << endl;
    cout << "   • Partículas: " << n << " con " << MATH_ITERATIONS << " iteraciones cada una" << endl;
    cout << "   • Estrellas: " << m << endl;
    cout << "   • Operaciones matemáticas: ~" << (n * MATH_ITERATIONS * 10) << " por frame" << endl;
    cout << endl;
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


void colorBH(float u, float v, float &r, float &g, float &b, float time_T){
    float c1=0.5f+0.5f*sinf(6.2831853f*(u+0.05f*time_T));
    float c2=0.5f+0.5f*sinf(6.2831853f*(u*0.5f+0.3f*v)+2.1f+0.1f*time_T);
    float c3=0.5f+0.5f*sinf(6.2831853f*(u*0.9f-0.2f*v)+3.6f-0.07f*time_T);
    float blue = 0.55f+0.45f*c1;
    float purple = 0.45f+0.55f*c2;
    float pink = 0.55f+0.45f*c3;
    r = 0.25f*blue + 0.35f*purple + 0.80f*pink;
    g = 0.35f*blue + 0.45f*purple + 0.30f*pink;
    b = 1.00f*blue + 0.60f*purple + 0.20f*pink;
}


void preCalculateStars(){
    auto calc_start = chrono::high_resolution_clock::now();
    
    #pragma omp parallel for num_threads(num_threads) schedule(static)
    for(int i = 0; i < (int)stars.size(); i++){
        auto &s = stars[i];
        float tw=0.65f+0.35f*sinf(0.6f*T+s.a*3.f);
        
        star_render_data[i].x = s.r;
        star_render_data[i].y = s.spd;
        star_render_data[i].z = s.z;
        star_render_data[i].r = 0.45f*tw;
        star_render_data[i].g = 0.55f*tw;
        star_render_data[i].b = 1.0f*tw;
        star_render_data[i].a = 1.0f;
        star_render_data[i].visible = true;
    }
    
    auto calc_end = chrono::high_resolution_clock::now();
    if (timing_enabled) {
        total_parallel_time += chrono::duration<double>(calc_end - calc_start).count();
    }
}

void drawStars(){
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE,GL_ONE);
    glPointSize(1.8f);
    glBegin(GL_POINTS);
    
    
    for(const auto &data : star_render_data){
        if(data.visible){
            glColor4f(data.r, data.g, data.b, data.a);
            glVertex3f(data.x, data.y, data.z);
        }
    }
    
    glEnd();
    glEnable(GL_DEPTH_TEST);
}


void preCalculateParticles(float znear, float zfar, float swirl, int pass_index){
    const float INNER_R = 10.0f;
    int base_index = pass_index * pts.size();
    
    #pragma omp parallel for num_threads(num_threads) schedule(dynamic, 50)
    for(int i = 0; i < (int)pts.size(); i++){
        auto &p = pts[i];
        int data_index = base_index + i;
        
        float z=p.z+fmodf(T*p.spd,1400.f);
        
        if(z>znear || z<zfar) {
            particle_render_data[data_index].visible = false;
            continue;
        }
        
        float a=p.a + swirl*T + 0.0019f*z + p.band*(6.2831853f/7.f);
        float r=p.r*(1.f+0.0011f*z);
        if(r<INNER_R) r=INNER_R;
        float wob=0.8f*sinf(0.7f*T+p.band*0.8f+0.02f*z);
        float x=(r+wob)*cosf(a)+p.jx*sinf(0.9f*T+0.01f*z);
        float y=(r-wob)*sinf(a)+p.jy*cosf(0.8f*T+0.013f*z);
        float u=fmodf(0.0025f*z + 0.12f*p.band,1.f);
        float v=fabsf(z)/1400.f;
        
        float cr,cg,cb; 
        colorBH(u,v,cr,cg,cb,T);
        
        float glow=0.6f+0.4f*sinf(2.4f*T+0.3f*p.band+0.003f*z);
        float centerFade = 0.6f + 0.4f*(r/INNER_R);
        float fade=(1.f - fminf(1.f,v))*glow*centerFade;
        
        particle_render_data[data_index].x = x;
        particle_render_data[data_index].y = y;
        particle_render_data[data_index].z = z;
        particle_render_data[data_index].r = cr;
        particle_render_data[data_index].g = cg;
        particle_render_data[data_index].b = cb;
        particle_render_data[data_index].a = fade;
        particle_render_data[data_index].visible = true;
    }
}

void pass(float ps, float alphaMul, float znear, float zfar, float swirl, float kdepth, int pass_index){
    auto calc_start = chrono::high_resolution_clock::now();
    
    
    preCalculateParticles(znear, zfar, swirl, pass_index);
    
    auto calc_end = chrono::high_resolution_clock::now();
    if (timing_enabled) {
        total_parallel_time += chrono::duration<double>(calc_end - calc_start).count();
    }
    
    
    glPointSize(ps);
    glBegin(GL_POINTS);
    
    int base_index = pass_index * pts.size();
    for(int i = 0; i < (int)pts.size(); i++){
        const auto &data = particle_render_data[base_index + i];
        if(data.visible){
            glColor4f(data.r, data.g, data.b, data.a * alphaMul);
            glVertex3f(data.x, data.y, data.z);
        }
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
    
    char fpsStr[400];
    sprintf(fpsStr, "FPS: %.1f | Hilos: %d | Partículas: %d | Matemática: %dx | Frames: %d | Tiempo: %.2fs | PARALELO", 
            currentFPS, num_threads, PARTICLE_COUNT, MATH_ITERATIONS, frame_count_timing, total_computation_time);
    
    glRasterPos2f(10, H - 25);
    for (char* c = fpsStr; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
    
    char infoStr[250];
    sprintf(infoStr, "Ops/frame: ~%d | Estrellas: %d | [+/-] Cambiar hilos | [ESC] Salir", 
            PARTICLE_COUNT * MATH_ITERATIONS * 10, STAR_COUNT);
    glRasterPos2f(10, H - 45);
    for (char* c = infoStr; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
    
    char controlStr[] = "[C] Camara | [F] FPS";
    glRasterPos2f(10, H - 65);
    for (char* c = controlStr; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
    
    if (camera.freeMode) {
        char moveStr[] = "WASD: Movimiento | QE: Arriba/Abajo | Mouse: Mirar";
        glRasterPos2f(10, H - 85);
        for (char* c = moveStr; *c; c++) {
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
    auto draw_start = chrono::high_resolution_clock::now();
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE,GL_ONE);
    glDisable(GL_LIGHTING);
    camera_control();
    
    
    preCalculateStars();
    drawStars();
    
    
    pass(1.5f,0.15f,1200.f,-3000.f,0.25f,0.0019f,0);
    pass(1.8f,0.25f,900.f,-2600.f,0.35f,0.0019f,1);
    pass(2.2f,0.40f,600.f,-2000.f,0.40f,0.0021f,2);
    pass(2.6f,0.50f,280.f,-1200.f,0.45f,0.0021f,3);
    pass(3.5f,0.75f,150.f,-800.f,0.50f,0.0023f,4);
    pass(4.2f,0.95f,80.f,-520.f,0.55f,0.0023f,5);
    
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    drawFPS();
    
    auto draw_end = chrono::high_resolution_clock::now();
    if (timing_enabled) {
        total_draw_time += chrono::duration<double>(draw_end - draw_start).count();
    }
}

void display(){
    if (timing_enabled) {
        frame_start = chrono::high_resolution_clock::now();
    }
    
    T=now();
    glClearColor(0.02f,0.02f,0.06f,1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    draw();
    glutSwapBuffers();
    
    if (timing_enabled) {
        frame_end = chrono::high_resolution_clock::now();
        total_computation_time += chrono::duration<double>(frame_end - frame_start).count();
        frame_count_timing++;
        
        
        if (frame_count_timing % 1000 == 0) {
            cout << "Frames procesados: " << frame_count_timing << 
                    ", Tiempo promedio por frame: " << 
                    (total_computation_time / frame_count_timing) * 1000 << " ms" << 
                    ", FPS actual: " << currentFPS << 
                    ", Hilos: " << num_threads << endl;
        }
    }
}

void reshape(int w,int h){ W=w; H=h; proj(); }
void idle(){ glutPostRedisplay(); }

void key(unsigned char k, int x, int y) {
    float moveSpeed = camera.speed;
    
    switch(k) {
        case 27: 
            saveTimingMetrics();
            exit(0); 
            break; 
        case 'c': case 'C': 
            camera.freeMode = !camera.freeMode;
            if (!camera.freeMode) {
                camera.x = 30.0f; camera.y = 5.0f; camera.z = 0.0f;
                camera.pitch = 0.0f; camera.yaw = 0.0f;
            }
            break;
        case '+': case '=':
            if(num_threads < omp_get_max_threads()) {
                num_threads++;
                cout << "Número de hilos aumentado a: " << num_threads << endl;
            }
            break;
        case '-': case '_':
            if(num_threads > 1) {
                num_threads--;
                cout << "Número de hilos reducido a: " << num_threads << endl;
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
    
    num_threads = omp_get_max_threads();
    
    cout << "SCREENSAVER PARALELO" << endl;
    cout << "============================================================" << endl;
    
    
    if(argc > 1) {
        PARTICLE_COUNT = atoi(argv[1]);
        if(PARTICLE_COUNT < 10000) PARTICLE_COUNT = 10000;
        if(PARTICLE_COUNT > 1000000) PARTICLE_COUNT = 1000000;
        cout << "• Partículas configuradas por argumento: " << PARTICLE_COUNT << endl;
    }
    
    if(argc > 2) {
        MATH_ITERATIONS = atoi(argv[2]);
        if(MATH_ITERATIONS < 1) MATH_ITERATIONS = 1;
        if(MATH_ITERATIONS > 50) MATH_ITERATIONS = 50;
        cout << "• Iteraciones matemáticas configuradas: " << MATH_ITERATIONS << endl;
    }
    
    if(argc > 3) {
        num_threads = atoi(argv[3]);
        if(num_threads < 1) num_threads = 1;
        if(num_threads > omp_get_max_threads()) num_threads = omp_get_max_threads();
        cout << "• Hilos configurados por argumento: " << num_threads << endl;
    }
    
    cout << endl;
    cout << "CONFIGURACIÓN:" << endl;
    cout << "   • Partículas: " << PARTICLE_COUNT << endl;
    cout << "   • Iteraciones matemáticas por partícula: " << MATH_ITERATIONS << endl;
    cout << "   • Estrellas: " << STAR_COUNT << endl;
    cout << "   • Carga computacional estimada: " << (PARTICLE_COUNT * MATH_ITERATIONS * 10) << " ops/frame" << endl;
    cout << "   • Hilos OpenMP: " << num_threads << " de " << omp_get_max_threads() << " disponibles" << endl;
    cout << "   • Versión: PARALELA" << endl;
    cout << "   • Passes de renderizado: 6" << endl;
    cout << endl;
    cout << "Iniciando medición de tiempo" << endl;
    
    start_time = chrono::high_resolution_clock::now();
    
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(W,H);
    glutCreateWindow("Agujero de gusano");
    proj(); 
    gen();
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