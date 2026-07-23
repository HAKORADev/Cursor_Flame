#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/XInput2.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <map>
#include <future>
#include <mutex>

namespace Cfg {
    constexpr int EDGE_MARGIN = 60;
    constexpr float FADE_POWER = 2.0f, EDGE_SOFT_THRESHOLD = 0.05f, PHYS_DRAG = 0.98f;
    static float PHYS_GRAVITY = 0.02f, PHYS_WOBBLE = 0.3f;
    constexpr float VELOCITY_THRESHOLD = 1.0f, VELOCITY_SPREAD = 0.5f;
    static int SPAWN_BASE = 7;
    constexpr float SPAWN_OFFSET_X = 15.0f, SPAWN_OFFSET_Y = 10.0f, SPAWN_SPEED_MIN = 2.0f, SPAWN_SPEED_MAX = 5.0f;
    constexpr float SPARK_MERGE_DIST = 25.0f, SPAWN_SCROLL_SENS = 2.0f, SPARK_DECAY = 0.11f;
    static int SMOKE_MAX = 30;
    constexpr float SMOKE_CHANCE = 0.02f, SMOKE_LIFE_THR = 0.3f, SMOKE_DECAY = 0.01f;
    constexpr float INTERP_THRESH = 20.0f, INTERP_STEP = 8.0f, WIND_DECAY = 0.95f;
    constexpr float BURNOUT_TOLERANCE = 2.5f, BURNOUT_SPEED_MIN = 1.0f, BURNOUT_SPEED_MAX = 3.0f;
    constexpr float FIREBALL_THR = 15.0f, FIREBALL_INHERIT = 0.6f, FIREBALL_LIFE = 1.5f;
    constexpr int FIREBALL_COUNT = 2, COLOR_ALPHA = 220;
}

struct KF { float t,r,g,b; };
struct C4 { float r,g,b,a; C4(float r=0,float g=0,float b=0,float a=255):r(r),g(g),b(b),a(a){} };
static C4 c4lerp(C4 a, C4 b, float t) { return {a.r+(b.r-a.r)*t, a.g+(b.g-a.g)*t, a.b+(b.b-a.b)*t, a.a+(b.a-a.a)*t}; }
static C4 c4clamp(C4 c) { return {std::min(255.f,std::max(0.f,c.r)), std::min(255.f,std::max(0.f,c.g)), std::min(255.f,std::max(0.f,c.b)), std::min(255.f,std::max(0.f,c.a))}; }

struct AppConfig {
    bool tail=1, on_click=1, on_hold=1, on_scroll=1, strike=1, interactive_edges=1;
    int quality=0, theme=0;
    float gravity_mult=1.0f, wind_x=0.0f, wind_y=0.0f, flicker_mult=1.0f;
    
    // Custom Quality
    int q_spawn_base=30, q_smoke_max=100, q_smoke_blend=1, q_render_style=1;
    float q_particle_size=1.0f, q_softness=2.5f, q_jitter_int=1.0f, particle_life_sec=1.0f;

    // SubEffects
    float overall_opacity=220.0f;
    float strike_r=200, strike_g=255, strike_b=255, strike_a=255;
    float click_r=255, click_g=255, click_b=200, click_a=255;
    float scroll_r=255, scroll_g=50, scroll_b=20, scroll_a=220;
    float tail_len_mult=1.0f, tail_thick_mult=1.0f, wave_amp=0.0f, wave_freq=0.1f;

    // Custom Theme
    KF custom_gradient[10];
    float gradient_speed=60.0f, gradient_reverse_speed=60.0f;

    void load(const std::string& p) {
        // Defaults
        tail=1; on_click=1; on_hold=1; on_scroll=1; strike=1; interactive_edges=1; quality=0; theme=0; gravity_mult=1.0f; wind_x=0.0f; wind_y=0.0f; flicker_mult=1.0f;
        q_spawn_base=30; q_smoke_max=100; q_smoke_blend=1; q_render_style=1; q_particle_size=1.0f; q_softness=2.5f; q_jitter_int=1.0f; particle_life_sec=1.0f;
        overall_opacity=220.0f; strike_r=200; strike_g=255; strike_b=255; strike_a=255; click_r=255; click_g=255; click_b=200; click_a=255; scroll_r=255; scroll_g=50; scroll_b=20; scroll_a=220;
        tail_len_mult=1.0f; tail_thick_mult=1.0f; wave_amp=0.0f; wave_freq=0.1f;
        gradient_speed=60.0f; gradient_reverse_speed=60.0f;
        for(int i=0; i<10; i++) { custom_gradient[i] = {(float)i * 6.0f, 255.f - i*20.f, 100.f + i*10.f, 50.f + i*20.f}; }

        std::ifstream f(p);
        if(!f.is_open()){
            std::ofstream out(p);
            out << "# KursorFlame Configuration File\n"
                << "# ==============================\n\n"
                << "[General]\n"
                << "# tail: Enable trailing particles when the cursor is moving.\n"
                << "# 1 = Enabled (Default), 0 = Disabled.\n" << "tail = 1\n\n"
                << "# on_click: Enable the special effect on every mouse click.\n"
                << "# 1 = Enabled (Default), 0 = Disabled.\n" << "on_click = 1\n\n"
                << "# on_hold: Enable high-intensity 'Burnout' flare when holding a mouse button.\n"
                << "# 1 = Enabled (Default), 0 = Disabled.\n" << "on_hold = 1\n\n"
                << "# on_scroll: Enable particle bursts when using the mouse scroll wheel.\n"
                << "# 1 = Enabled (Default), 0 = Disabled.\n" << "on_scroll = 1\n\n"
                << "# strike: Enable blue/cyan flash effect when clicking while moving fast.\n"
                << "# 1 = Enabled (Default), 0 = Disabled.\n" << "strike = 1\n\n"
                << "# interactive_edges: Make particles bounce off the screen borders.\n"
                << "# 1 = Enabled (Default), 0 = Disabled.\n" << "interactive_edges = 1\n\n"
                << "[Visuals]\n"
                << "# quality: Sets the rendering style and particle density.\n"
                << "# 0 = Low, 1 = Medium, 2 = High, 3 = Ultra, 4 = Custom.\n" << "quality = 0\n\n"
                << "# theme: Selects the overall look and physics behavior.\n"
                << "# 0 = Fire (Classic), 1 = Snow, 2 = Water, 4 = Custom (Uses [CustomTheme]).\n" << "theme = 0\n\n"
                << "[Physics]\n"
                << "# gravity_mult: Scales vertical pull. Negatives pull UP, Positives DOWN.\n"
                << "# Supports floating point values like -5.5 or 10.0.\n" << "gravity_mult = 1.0\n\n"
                << "# flicker_mult: Scales particle jitter and chaotic movement intensity.\n"
                << "# Min: 0.0 (Still), Max: 10.0 (Chaotic).\n" << "flicker_mult = 1.0\n\n"
                << "# wind_x/y: Constant environmental push in X and Y directions.\n"
                << "# Negatives supported (e.g., wind_x = -2.0 pushes left).\n" << "wind_x = 0.0\n" << "wind_y = 0.0\n\n"
                << "[Quality]\n"
                << "# Custom profile settings (Used only if quality = 4).\n\n"
                << "# q_spawn_base: Base number of particles spawned per tick.\n"
                << "# Min: 0, Max: 5000.\n" << "q_spawn_base = 30\n\n"
                << "# q_smoke_max: Maximum number of active smoke particles.\n"
                << "# Min: 0, Max: 5000.\n" << "q_smoke_max = 100\n\n"
                << "# particle_life_sec: Base time in seconds for a particle to die.\n"
                << "# Min: 0.0, Max: 600.0.\n" << "particle_life_sec = 1.0\n\n"
                << "# q_particle_size: Global multiplier for particle size.\n"
                << "# Min: 0.0, Max: 10.0.\n" << "q_particle_size = 1.0\n\n"
                << "# q_softness: Softness/Blur radius for particles.\n"
                << "# Min: 0.0, Max: 15.0.\n" << "q_softness = 2.5\n\n"
                << "# q_jitter_int: Intensity of the flicker movement for custom quality.\n"
                << "# Min: 0.0, Max: 10.0.\n" << "q_jitter_int = 1.0\n\n"
                << "# q_smoke_blend: Blending mode for smoke.\n"
                << "# 0 = Additive (Bright), 1 = Alpha (Soft).\n" << "q_smoke_blend = 1\n\n"
                << "# q_render_style: Particle shape and rendering method.\n"
                << "# 0 = Teardrop (Classic), 1 = Soft Blob (Modern), 2 = Shaded (Fluid).\n" << "q_render_style = 1\n\n"
                << "[SubEffects]\n"
                << "# overall_opacity: Master alpha channel for all particles globally.\n"
                << "# Min: 0 (Invisible), Max: 255 (Opaque).\n" << "overall_opacity = 220\n\n"
                << "# strike: High-speed click color (R, G, B, Alpha: 0-255)\n"
                << "strike_r = 200\nstrike_g = 255\nstrike_b = 255\nstrike_a = 255\n\n"
                << "# click: Standard click color (R, G, B, Alpha: 0-255)\n"
                << "click_r = 255\nclick_g = 255\nclick_b = 200\nclick_a = 255\n\n"
                << "# scroll: Mouse scroll burst color (R, G, B, Alpha: 0-255)\n"
                << "scroll_r = 255\nscroll_g = 50\nscroll_b = 20\nscroll_a = 220\n\n"
                << "# Tail Dynamics (Affects Custom Quality only)\n"
                << "# tail_len_mult: Multiplier for particle lifespan (Trail length).\n"
                << "# Min: 0.1, Max: 10.0.\n" << "tail_len_mult = 1.0\n\n"
                << "# tail_thick_mult: Scales the thickness of the trail particles.\n"
                << "# Min: 0.1, Max: 10.0.\n" << "tail_thick_mult = 1.0\n\n"
                << "# wave_amp: Intensity of sinusoidal oscillation. Negatives supported.\n"
                << "# Min: 0.0, Max: 100.0.\n" << "wave_amp = 0.0\n\n"
                << "# wave_freq: Speed/Frequency of trail oscillation.\n"
                << "# Min: 0.0, Max: 5.0.\n" << "wave_freq = 0.1\n\n"
                << "[CustomTheme]\n"
                << "# 10-Level Color Gradient (Used only if theme = 4).\n"
                << "# gradient_speed: Frames for forward cycle (Level 0 to 9).\n"
                << "# Min: 1.0.\n" << "gradient_speed = 60.0\n\n"
                << "# gradient_reverse_speed: Frames for backward cycle (Level 9 to 0).\n"
                << "# Min: 1.0.\n" << "gradient_reverse_speed = 60.0\n";
            for(int i=0; i<10; i++) out << "\n# Level " << i << " Color (0-255)\nl" << i << "_r = " << (int)custom_gradient[i].r << "\nl" << i << "_g = " << (int)custom_gradient[i].g << "\nl" << i << "_b = " << (int)custom_gradient[i].b << "\n";
            return;
        }
        std::string l;
        while(std::getline(f,l)){
            if(l.empty()||l[0]=='#'||l[0]=='[')continue;
            size_t s=l.find('='); if(s==std::string::npos)continue;
            std::string k=l.substr(0,s), v=l.substr(s+1);
            auto tr=[](std::string& t){ t.erase(0,t.find_first_not_of(" \t\r\n")); t.erase(t.find_last_not_of(" \t\r\n")+1); };
            tr(k); tr(v);
            try{
                if(k=="tail")tail=(v=="1"); else if(k=="on_click")on_click=(v=="1"); else if(k=="on_hold")on_hold=(v=="1"); else if(k=="on_scroll")on_scroll=(v=="1"); else if(k=="strike")strike=(v=="1"); else if(k=="interactive_edges")interactive_edges=(v=="1");
                else if(k=="quality")quality=std::clamp(std::stoi(v),0,4); else if(k=="theme")theme=std::stoi(v);
                else if(k=="gravity_mult")gravity_mult=std::stof(v); else if(k=="flicker_mult")flicker_mult=std::stof(v); else if(k=="wind_x")wind_x=std::stof(v); else if(k=="wind_y")wind_y=std::stof(v);
                else if(k=="q_spawn_base")q_spawn_base=std::clamp(std::stoi(v),0,5000); else if(k=="q_smoke_max")q_smoke_max=std::clamp(std::stoi(v),0,5000);
                else if(k=="particle_life_sec")particle_life_sec=std::clamp(std::stof(v),0.0f,600.0f); else if(k=="q_particle_size")q_particle_size=std::stof(v); else if(k=="q_softness")q_softness=std::stof(v); else if(k=="q_jitter_int")q_jitter_int=std::stof(v);
                else if(k=="q_smoke_blend")q_smoke_blend=std::stoi(v); else if(k=="q_render_style")q_render_style=std::stoi(v);
                else if(k=="overall_opacity")overall_opacity=std::clamp(std::stof(v),0.0f,255.0f);
                else if(k=="strike_r")strike_r=std::stof(v); else if(k=="strike_g")strike_g=std::stof(v); else if(k=="strike_b")strike_b=std::stof(v); else if(k=="strike_a")strike_a=std::stof(v);
                else if(k=="click_r")click_r=std::stof(v); else if(k=="click_g")click_g=std::stof(v); else if(k=="click_b")click_b=std::stof(v); else if(k=="click_a")click_a=std::stof(v);
                else if(k=="scroll_r")scroll_r=std::stof(v); else if(k=="scroll_g")scroll_g=std::stof(v); else if(k=="scroll_b")scroll_b=std::stof(v); else if(k=="scroll_a")scroll_a=std::stof(v);
                else if(k=="tail_len_mult")tail_len_mult=std::stof(v); else if(k=="tail_thick_mult")tail_thick_mult=std::stof(v);
                else if(k=="wave_amp")wave_amp=std::stof(v); else if(k=="wave_freq")wave_freq=std::stof(v);
                else if(k=="gradient_speed")gradient_speed=std::stof(v); else if(k=="gradient_reverse_speed")gradient_reverse_speed=std::stof(v);
                else {
                    for(int i=0; i<10; i++) {
                        std::string prefix = "l" + std::to_string(i) + "_";
                        if(k == prefix + "r") custom_gradient[i].r = std::stof(v);
                        else if(k == prefix + "g") custom_gradient[i].g = std::stof(v);
                        else if(k == prefix + "b") custom_gradient[i].b = std::stof(v);
                    }
                }
            }catch(...){}
        }
        for(int i=0; i<10; i++) custom_gradient[i].t = (float)i * (gradient_speed / 9.0f);
        if(quality==0){Cfg::SPAWN_BASE=7;Cfg::SMOKE_MAX=30;}else if(quality==1){Cfg::SPAWN_BASE=15;Cfg::SMOKE_MAX=60;}else if(quality==2){Cfg::SPAWN_BASE=30;Cfg::SMOKE_MAX=100;}else if(quality==3){Cfg::SPAWN_BASE=50;Cfg::SMOKE_MAX=200;}else{Cfg::SPAWN_BASE=q_spawn_base;Cfg::SMOKE_MAX=q_smoke_max;}
    }
};

enum class State { NORMAL, BURNOUT, FIREBALL };
enum class BurnoutPhase { NONE, PHASE_1, PHASE_2, PHASE_3, PHASE_4 };
static std::mt19937 g_rng([]{ std::random_device rd; return rd(); }());
static inline float randf(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(g_rng); }
static inline float randf01() { return std::uniform_real_distribution<float>(0.0f, 1.0f)(g_rng); }

static const KF KEYFRAMES[] = { {0.f,255,80,0},{2.f,255,120,0},{4.f,255,180,0},{6.f,100,150,255},{30.f,150,50,200},{60.f,10,10,15} };
static C4 cmatrix_get(float e, const KF* kfs, int count, float max_t) {
    if(e >= max_t) return {kfs[count-1].r, kfs[count-1].g, kfs[count-1].b, 255};
    for(int i=0; i<count-1; i++){
        if(e < kfs[i+1].t){
            float f = (e - kfs[i].t) / (kfs[i+1].t - kfs[i].t);
            return {kfs[i].r + (kfs[i+1].r - kfs[i].r)*f, kfs[i].g + (kfs[i+1].g - kfs[i].g)*f, kfs[i].b + (kfs[i+1].b - kfs[i].b)*f, 255};
        }
    }
    return {kfs[count-1].r, kfs[count-1].g, kfs[count-1].b, 255};
}

struct PMatrix {
    static constexpr int SIZE = 30000; float x[SIZE],y[SIZE],vx[SIZE],vy[SIZE],life[SIZE],max_life[SIZE],size_[SIZE],wave_offset[SIZE]; int active[SIZE],is_scroll[SIZE];
    PMatrix(){memset(this,0,sizeof(*this));}
    void spawn(int i,float x_,float y_,float vx_,float vy_,float l,float sz,bool s=false){x[i]=x_;y[i]=y_;vx[i]=vx_;vy[i]=vy_;life[i]=l;max_life[i]=l;size_[i]=sz;active[i]=1;is_scroll[i]=s?1:0;wave_offset[i]=randf(0,6.28f);}
    void kill(int i){active[i]=0;}
    bool is_active(int i){return active[i]==1;}
    int count()const{int c=0;for(int i=0;i<SIZE;i++)c+=active[i];return c;}
};
struct SparkPool { static constexpr int MAX=200; float x[MAX],y[MAX],life[MAX]; int active[MAX]; SparkPool(){memset(this,0,sizeof(*this));} void spawn(int i,float cx,float cy){x[i]=cx+randf(-30,30);y[i]=cy+randf(-25,35);life[i]=1.f;active[i]=1;} void kill(int i){active[i]=0;} void update(int i){if(!active[i])return;life[i]-=0.11f;x[i]+=randf(-2,2)+sinf(life[i]*20)*2.5f;y[i]+=randf(-3,1);if(life[i]<=0)kill(i);}};
struct SmokePool { static constexpr int MAX=10000; float x[MAX],y[MAX],life[MAX],sz[MAX]; int active[MAX]; SmokePool(){memset(this,0,sizeof(*this));} void spawn(int i,float cx,float cy,float s){x[i]=cx;y[i]=cy;life[i]=1.f;sz[i]=s;active[i]=1;} void kill(int i){active[i]=0;} void update(int i){if(!active[i])return;life[i]-=0.01f;sz[i]+=0.02f;y[i]-=randf(0.5f,1.5f);x[i]+=randf(-0.5f,0.5f);if(life[i]<=0)kill(i);} int count()const{int c=0;for(int i=0;i<MAX;i++)c+=active[i];return c;}};
static int g_W=0,g_H=0; static std::vector<uint32_t> g_buf;
static void renderClear(){memset(g_buf.data(),0,g_buf.size()*4);}
static inline void blendAdd(int px,int py,float r,float g,float b,float a){ if(px<0||px>=g_W||py<0||py>=g_H)return; int ia=(int)a; if(ia<=0)return; uint32_t* d=&g_buf[py*g_W+px]; uint8_t* p=(uint8_t*)d; p[0]=(uint8_t)std::min(255,(int)p[0]+(int)b*ia/255); p[1]=(uint8_t)std::min(255,(int)p[1]+(int)g*ia/255); p[2]=(uint8_t)std::min(255,(int)p[2]+(int)r*ia/255); p[3]=(uint8_t)std::min(255,(int)p[3]+ia); }
static inline void blendAlpha(int px,int py,float r,float g,float b,float a){ if(px<0||px>=g_W||py<0||py>=g_H)return; float fa=a/255.f; if(fa<=0.f)return; uint32_t* d=&g_buf[py*g_W+px]; uint8_t* p=(uint8_t*)d; p[0]=(uint8_t)(p[0]*(1.f-fa)+b*fa); p[1]=(uint8_t)(p[1]*(1.f-fa)+g*fa); p[2]=(uint8_t)(p[2]*(1.f-fa)+r*fa); }
static C4 evalTeardropGrad(const C4& s0,const C4& s1,const C4& s2,float t){ if(t<=0.f)return s0; if(t<=0.4f)return c4lerp(s0,s1,t/0.4f); if(t<=0.7f)return c4lerp(s1,s2,(t-0.4f)/0.3f); if(t<=1.f)return c4lerp(s2,C4(0,0,0,0),(t-0.7f)/0.3f); return C4(0,0,0,0); }

class KursorFlame {
public:
    PMatrix p; SparkPool sparks; SmokePool smoke; AppConfig config;
    float cx=0,cy=0,px=0,py=0,vx=0,vy=0,speed=0; int screen_width=0,screen_height=0;
    State state=State::NORMAL; bool mouse_down=0; uint64_t hold_start=0; float hold_duration=0,max_hold_duration=10000;
    float wind_x=0,wind_y=0; int wind_timer=0;
    bool burnout_active=0; float burnout_transition=0; BurnoutPhase burnout_phase=BurnoutPhase::NONE; float phase_start_time=0; int burnout_cycle_count=0;
    bool lightning_active=0; int lightning_frame=0,lightning_max_frames=8; float lightning_radius=0;
    C4 color_override{0,0,0,0}; float color_override_t=0,color_decay=0; int pidx=0,sidx=0,smidx=0;
    int duration_start = -1, frame_count = 0;
    KursorFlame(){config.load("kursor.conf");}
    uint64_t get_now_ms(){auto n=std::chrono::steady_clock::now(); return std::chrono::duration_cast<std::chrono::milliseconds>(n.time_since_epoch()).count();}
    void init(int sw,int sh,float ix,float iy){screen_width=sw;screen_height=sh;cx=px=ix;cy=py=iy;}
    void on_move(float x,float y){cx=x;cy=y;}
    void _spawn_spark(float x, float y) { sparks.spawn(sidx, x, y); sidx=(sidx+1)%SparkPool::MAX; }
    void _spawn_smoke(float x, float y) { smoke.spawn(smidx, x, y, randf(10,15)); smidx=(smidx+1)%SmokePool::MAX; }
    void _spawn_particle(float x,float y,float ivx,float ivy,float it,float lm,bool s=false){
        int i=pidx; pidx=(pidx+1)%PMatrix::SIZE; float a;
        if(config.theme==1) a = randf(0.5f, 2.5f); else if(config.theme==2) a = randf(0, 6.28f);
        else a = (sqrtf(ivx*ivx+ivy*ivy)<1.f)?randf(-2.2f,-0.9f):atan2f(-ivy,-ivx)+randf(-0.5f,0.5f);
        float psz = (config.quality == 4) ? config.q_particle_size * config.tail_thick_mult : 1.0f;
        float sp = randf(2,5)*it;
        float decay = (config.quality == 4) ? (1.0f / (config.particle_life_sec * 60.0f * config.tail_len_mult * (1.0f/lm) + 0.001f)) : (randf(0.015f,0.035f)*lm*(config.theme==1?3.5f:1.f));
        p.spawn(i,x,y,cosf(a)*sp*randf(0.3f,0.8f),sinf(a)*sp,1.f,randf(8,20)*it*psz,s);
        p.max_life[i]=decay;
    }
    void _spawn_scroll_particle(float x,float y,float svy){int i=pidx;pidx=(pidx+1)%PMatrix::SIZE;float psz=(config.quality==4)?config.q_particle_size*config.tail_thick_mult:1.0f;p.spawn(i,x,y,randf(-1,1)*fabsf(svy),svy+randf(-0.5f,0.5f),1.f,randf(10,18)*psz,true);p.max_life[i]=randf(0.02f,0.04f);}
    void _start_lightning_effect(){
        if(config.theme==1){ for(int i=0;i<40;i++){float a=randf(0,6.28f),s=randf(2,8);_spawn_particle(cx,cy,cosf(a)*s,sinf(a)*s,1.5f,2.f);} }
        else if(config.theme==2){ for(int i=0;i<30;i++){float a=randf(0,6.28f),s=randf(1,5);_spawn_particle(cx,cy,cosf(a)*s,sinf(a)*s,1.2f,1.5f);} }
        else{ lightning_active=1;lightning_frame=0;for(int i=0;i<20;i++){float a=randf(0,6.283f),s=randf(8,12);_spawn_particle(cx,cy,cosf(a)*s,sinf(a)*s,1.5f,2.f);}for(int i=0;i<15;i++)_spawn_spark(cx,cy);for(int i=0;i<8;i++)_spawn_smoke(cx,cy); }
    }
    void on_click(bool pr){
        if(pr&&!mouse_down){
            hold_start=get_now_ms(); bool is_m=(speed>Cfg::VELOCITY_THRESHOLD*2.f);
            if(is_m){ if(config.strike){ color_override=C4(config.strike_r, config.strike_g, config.strike_b, config.strike_a); color_override_t=1.f; color_decay=0.033f;
                    if(config.theme==1){ for(int i=0;i<25;i++){_spawn_particle(cx,cy,randf(-2,2),randf(-15,15),2.f,1.5f);} }
                    else if(config.theme==2){ for(int i=0;i<25;i++){_spawn_particle(cx,cy,randf(-5,5),randf(5,15),1.5f,1.5f);} }
                    else{ for(int i=0;i<20;i++){float a=randf(0,6.283f),s=randf(10,20);_spawn_particle(cx,cy,cosf(a)*s,sinf(a)*s,2.f,1.2f);}}}}
            else{ if(config.on_click){_start_lightning_effect(); color_override=C4(config.click_r, config.click_g, config.click_b, config.click_a); color_override_t=1.f; color_decay=0.15f;}}
            burnout_cycle_count=0;burnout_phase=BurnoutPhase::NONE;phase_start_time=0;
        }else if(!pr&&mouse_down){burnout_active=0;burnout_transition=0;state=State::NORMAL;burnout_phase=BurnoutPhase::NONE;phase_start_time=0;}
        mouse_down=pr;
    }
    void on_scroll(int dy){if(!config.on_scroll)return; if(dy!=0){float svy=-dy*Cfg::SPAWN_SCROLL_SENS; color_override=C4(config.scroll_r, config.scroll_g, config.scroll_b, config.scroll_a); color_override_t=1.f; color_decay=0.1f; for(int i=0;i<8;i++)_spawn_scroll_particle(cx+randf(-30,30),cy+randf(-30,30),svy);}}
    void cursor_tick(){vx=cx-px;vy=cy-py;speed=sqrtf(vx*vx+vy*vy);px=cx;py=cy;_update_state();}
    void _interp_spawn(){if(!config.tail||speed<20.f)return;int st=std::min((int)(speed/8.f),50);for(int i=0;i<st;i++){float t=(i+1.f)/(st+1.f);_spawn_particle(cx-vx*t,cy-vy*t,vx,vy,0.6f+std::min(0.9f,speed/25.f),1.f);}}
    void _spawn_normal(){if(!config.tail&&speed>1.f)return;int c=(int)(Cfg::SPAWN_BASE*std::min(2.5f,speed/12.f));for(int i=0;i<c;i++)_spawn_particle(cx+randf(-15,15),cy+randf(-10,10),vx,vy,0.6f+std::min(0.9f,speed/25.f),1.f);}
    void _spawn_burnout(){ if(!config.on_hold)return; float theme_vy = 0; if(config.theme==1) theme_vy = -randf(2,5); else if(config.theme==2) theme_vy = randf(2,5);
        if(burnout_phase==BurnoutPhase::NONE){ float hp=hold_duration/10000.f;for(int i=0;i<(int)(Cfg::SPAWN_BASE*2*(0.5f+hp*1.5f));i++){ float a=randf(0,6.28f),s=randf(1,3)*(1.f+hp*2.f); _spawn_particle(cx+randf(-5,5),cy+randf(-5,5),cosf(a)*s,sinf(a)*s+theme_vy,1.2f,0.5f);}}}
    void _spawn_fireball(){int c=Cfg::SPAWN_BASE*2;float ma=(fabsf(vx)>0.1f||fabsf(vy)>0.1f)?atan2f(vy,vx):-1.57f;for(int i=0;i<c;i++){float a=ma+3.14f+randf(-0.5f,0.5f),s=randf(2,5);_spawn_particle(cx+randf(-15,15),cy+randf(-15,15),cosf(a)*s+vx*0.6f,sinf(a)*s+vy*0.6f,1.3f,1.f/1.5f);}}
    void _spawn_mode(){switch(state){case State::NORMAL:_spawn_normal();break;case State::BURNOUT:_spawn_burnout();break;case State::FIREBALL:_spawn_fireball();break;}}
    void _update_state(){uint64_t n=get_now_ms();if(mouse_down){hold_duration=std::min(10000.f,(float)(n-hold_start));if(speed>15.f){state=State::FIREBALL;burnout_phase=BurnoutPhase::NONE;}else if(fabsf(vx)>2.5f||fabsf(vy)>2.5f){state=State::NORMAL;burnout_phase=BurnoutPhase::NONE;}else{state=State::BURNOUT;_update_burnout_phase();}}else{hold_duration=0;state=State::NORMAL;burnout_phase=BurnoutPhase::NONE;}}
    void _update_burnout_phase(){if(phase_start_time==0)phase_start_time=hold_duration;float pd=hold_duration-phase_start_time;if(burnout_cycle_count<5){if(pd>=5000.f){burnout_phase=BurnoutPhase::PHASE_1;burnout_cycle_count++;phase_start_time=hold_duration;}else burnout_phase=BurnoutPhase::NONE;}else{switch(burnout_phase){case BurnoutPhase::NONE:burnout_phase=BurnoutPhase::PHASE_1;phase_start_time=hold_duration;break;case BurnoutPhase::PHASE_1:if(pd>=10000.f){burnout_phase=BurnoutPhase::PHASE_2;phase_start_time=hold_duration;}break;case BurnoutPhase::PHASE_2:if(pd>=15000.f){burnout_phase=BurnoutPhase::PHASE_3;phase_start_time=hold_duration;}break;case BurnoutPhase::PHASE_3:if(pd>=5000.f){burnout_phase=BurnoutPhase::PHASE_4;phase_start_time=hold_duration;burnout_cycle_count=0;}break;default:break;}}}
    void _merge_sparks(){for(int si=0;si<200;si++){if(!sparks.active[si])continue;for(int pi=0;pi<PMatrix::SIZE;pi++){if(!p.is_active(pi))continue;float dx=sparks.x[si]-p.x[pi],dy=sparks.y[si]-p.y[pi];if(dx*dx+dy*dy<625.f){p.life[pi]=std::min(1.f,p.life[pi]+0.15f);p.size_[pi]=std::min(p.max_life[pi]*1.5f,p.size_[pi]*1.2f);sparks.kill(si);break;}}}}
    void _emit_smoke(){if(smoke.count()>=Cfg::SMOKE_MAX)return;for(int i=0;i<PMatrix::SIZE;i++){if(p.is_active(i)&&p.life[i]<0.3f&&randf01()<0.02f){_spawn_smoke(p.x[i],p.y[i]);}}}
    void update_tick(){
        frame_count++; if(color_override_t>0){color_override_t-=color_decay;if(color_override_t<=0){color_override_t=0;color_override=C4(0,0,0,0);}}
        if(lightning_active){lightning_frame++;if(lightning_frame>=8){lightning_active=0;}else if(lightning_frame<6){lightning_radius=lightning_frame*25.f;int rc=15-lightning_frame*2;for(int i=0;i<rc;i++){float a=randf(0,6.28f),r=lightning_radius*randf(0.8f,1.2f);_spawn_particle(cx+cosf(a)*r,cy+sinf(a)*r,0,0,1.2f,1.f);}}}
        if(wind_timer>0){wind_timer--;wind_x*=0.95f;wind_y*=0.95f;}else{wind_x=wind_y=0;}
        if(p.count()>0){if(duration_start<0)duration_start=frame_count;}else{duration_start=-1;}
        _interp_spawn();_spawn_mode();
        float bg=(config.theme==1)?0.015f*config.gravity_mult:(config.theme==2)?0.002f*config.gravity_mult:-0.025f*config.gravity_mult;
        float tr=(config.quality==4)?(0.1f*config.q_jitter_int*config.flicker_mult):(config.quality>0?(0.1f*config.quality*config.flicker_mult):0.f);
        for(int i=0;i<PMatrix::SIZE;i++){
            if(!p.is_active(i))continue; p.life[i]-=p.max_life[i]; if(tr>0){p.vx[i]+=randf(-tr,tr);p.vy[i]+=randf(-tr,tr);}
            p.vx[i]+=config.wind_x;p.vy[i]+=config.wind_y;p.x[i]+=p.vx[i]+wind_x;p.y[i]+=p.vy[i]+wind_y;
            p.x[i]+=sinf(frame_count*0.1f+i)*0.3f*config.flicker_mult; 
            if(config.quality==4 && config.wave_amp != 0.0f) { p.x[i] += sinf(p.life[i] * 10.0f * config.wave_freq + p.wave_offset[i]) * config.wave_amp; }
            p.vx[i]*=0.98f; p.vy[i]*=0.98f; p.vy[i]+=bg;
            if(config.interactive_edges){if(p.x[i]<0){p.x[i]=0;p.vx[i]=fabsf(p.vx[i])*0.5f;}if(p.x[i]>screen_width){p.x[i]=screen_width;p.vx[i]=-fabsf(p.vx[i])*0.5f;}if(p.y[i]<0){p.y[i]=0;p.vy[i]=fabsf(p.vy[i])*0.5f;}if(p.y[i]>screen_height){p.y[i]=screen_height;p.vy[i]=-fabsf(p.vy[i])*0.5f;}}
            if(p.life[i]<=0)p.kill(i);
        }
        for(int i=0;i<SparkPool::MAX;i++)sparks.update(i); for(int i=0;i<SmokePool::MAX;i++) smoke.update(i);
        _merge_sparks();_emit_smoke();
    }
    void render(){_render_cpu_dual_core();}
private:
    void _render_cpu_dual_core(){renderClear();int mid=g_H/2;auto f1=std::async(std::launch::async,[&](){_render_cpu_region(0,mid);});auto f2=std::async(std::launch::async,[&](){_render_cpu_region(mid,g_H);});f1.get();f2.get();}
    void _render_cpu_region(int y0,int y1){
        for(int i=0;i<SmokePool::MAX;i++){if(smoke.active[i]&&smoke.y[i]+40>=y0&&smoke.y[i]-40<=y1)_drawSmokeRegion(smoke.x[i],smoke.y[i],smoke.life[i],smoke.sz[i],y0,y1);}
        for(int i=0;i<PMatrix::SIZE;i++){ if(p.is_active(i)&&p.y[i]+40>=y0&&p.y[i]-40<=y1){ float l=p.life[i],e=_edge_fade(p.x[i],p.y[i]); if(e<0.05f)continue; C4 c=_color(l,1.f,e,p.is_scroll[i]); float sz=std::max(1.f,p.size_[i]); 
            int cur_style = (config.quality == 4) ? config.q_render_style : (config.quality == 0 ? 0 : 1);
            if(cur_style==0)_drawTeardropRegion(p.x[i],p.y[i],sz*1.4f,sz*2.2f,c,0.8f+l*0.4f,p.vx[i],p.vy[i],y0,y1); 
            else if(cur_style==1)_drawSoftBlobRegion(p.x[i],p.y[i],sz*((config.quality==4)?config.q_softness:(config.quality==1?1.5f:2.5f)),c,y0,y1); 
            else _drawShadedRegion(p.x[i],p.y[i],sz*((config.quality==4)?config.q_softness:2.5f)*1.5f,c,y0,y1); } }
        for(int i=0;i<SparkPool::MAX;i++){if(sparks.active[i]&&sparks.y[i]+10>=y0&&sparks.y[i]-10<=y1)_drawSparkRegion(sparks.x[i],sparks.y[i],sparks.life[i],1.f,y0,y1);}
    }
    void _drawSoftBlobRegion(float x,float y,float s,const C4& c,int ry0,int ry1){
        if(s<=0.f)return; int iy0=std::max(ry0,(int)(y-s)),iy1=std::min(ry1-1,(int)(y+s+1)),ix0=std::max(0,(int)(x-s)),ix1=std::min(g_W-1,(int)(x+s+1));
        float r2=s*s,ir2=1.f/r2;for(int py=iy0;py<=iy1;py++){float dy2=(py+0.5f-y)*(py+0.5f-y);for(int px=ix0;px<=ix1;px++){float d2=(px+0.5f-x)*(px+0.5f-x)+dy2;if(d2<r2){float it=powf(1.f-sqrtf(d2*ir2),2.f);blendAdd(px,py,c.r,c.g,c.b,c.a*it);}}}
    }
    void _drawShadedRegion(float x,float y,float s,const C4& c,int ry0,int ry1){
        if(s<=0.f)return; int iy0=std::max(ry0,(int)(y-s)),iy1=std::min(ry1-1,(int)(y+s+1)),ix0=std::max(0,(int)(x-s)),ix1=std::min(g_W-1,(int)(x+s+1));
        float r2=s*s,ir2=1.f/r2;for(int py=iy0;py<=iy1;py++){float dy2=(py+0.5f-y)*(py+0.5f-y);for(int px=ix0;px<=ix1;px++){float d2=(px+0.5f-x)*(px+0.5f-x)+dy2;if(d2<r2){float it=1.f-sqrtf(d2*ir2);blendAdd(px,py,c.r,c.g,c.b,c.a*it);}}}
    }
    void _drawSmokeRegion(float sx,float sy,float l,float s,int ry0,int ry1){
        bool alpha = (config.quality == 4) ? (config.q_smoke_blend == 1) : (config.quality > 1);
        float a=l*(alpha?40.f:60.f);int iy0=std::max(ry0,(int)(sy-s)),iy1=std::min(ry1-1,(int)(sy+s+1)),ix0=std::max(0,(int)(sx-s)),ix1=std::min(g_W-1,(int)(sx+size_t(s)+1));
        float r2=s*s;for(int py=iy0;py<=iy1;py++){float dy2=(py+0.5f-sy)*(py+0.5f-sy);for(int px=ix0;px<=ix1;px++){float d2=(px+0.5f-sx)*(px+0.5f-sx)+dy2;if(d2<r2){float it=1.f-sqrtf(d2)/s;if(alpha)blendAlpha(px,py,40,40,40,a*it);else blendAdd(px,py,80,80,80,a*it);}}}
    }
    void _drawSparkRegion(float x,float y,float l,float it,int ry0,int ry1){
        float a=l*255.f,r=std::max(1.f,l*8.f)*2.f;int iy0=std::max(ry0,(int)(y-r)),iy1=std::min(ry1-1,(int)(y+r+1)),ix0=std::max(0,(int)(x-r)),ix1=std::min(g_W-1,(int)(x+r+1));
        for(int py=iy0;py<=iy1;py++){float dy2=(py+0.5f-y)*(py+0.5f-y);for(int px=ix0;px<=ix1;px++){float d2=(px+0.5f-x)*(px+0.5f-x)+dy2;float t=sqrtf(d2)/r;if(t<1.f){float ca=(t<0.5f)?a*1.3f+(a-a*1.3f)*(t/0.5f):(t<0.8f)?a+(a*0.2f-a)*((t-0.5f)/0.3f):a*0.2f*(1.f-(t-0.8f)/0.2f);blendAdd(px,py,255,220,180,std::clamp(ca,0.f,255.f));}}}
    }
    void _drawTeardropRegion(float x,float y,float w,float h,const C4& c,float ds,float pvx,float pvy,int ry0,int ry1){
        float gcx=x,gcy=y+h*0.15f,s=1.8f*ds,hw=w/2.f,hh=h/2.f;if(fabsf(pvx)>0.1f||fabsf(pvy)>0.1f){float a=atan2f(-pvy,-pvx);gcx+=cosf(a)*hw*0.3f;gcy+=sinf(a)*hh*0.3f;}
        float ty=gcy-hh*s,by=gcy+hh*0.5f,tr=hw*0.15f;float pts[6][2]={{gcx,ty},{gcx-hw+tr,gcy-hh*0.3f*s},{gcx-hw,by},{gcx,by+hh*0.2f},{gcx+hw,by},{gcx+hw-tr,gcy-hh*0.3f*s}};
        float minX=pts[0][0],maxX=pts[0][0],minY=pts[0][1],maxY=pts[0][1];for(int i=1;i<6;i++){minX=std::min(minX,pts[i][0]);maxX=std::max(maxX,pts[i][0]);minY=std::min(minY,pts[i][1]);maxY=std::max(maxY,pts[i][1]);}
        int iy0=std::max(ry0,(int)minY-1),iy1=std::min(ry1-1,(int)maxY+1);float ig=hw*1.2f>0.f?1.f/(hw*1.2f):0.f;C4 s0(c.r,c.g,c.b,std::min(255.f,c.a*1.2f)),s1=c,s2(c.r,c.g,c.b,c.a*0.3f);
        for(int iy=iy0;iy<=iy1;iy++){float fy=iy+0.5f,xs[8];int nx=0;for(int i=0;i<6;i++){int j=(i+1)%6;float ya=pts[i][1],yb=pts[j][1],xa=pts[i][0],xb=pts[j][0];if((fy>=ya&&fy<yb)||(fy>=yb&&fy<ya)){float t=(fy-ya)/(yb-ya);if(nx<8)xs[nx++]=xa+t*(xb-xa);}}if(nx<2)continue;std::sort(xs,xs+nx);for(int k=0;k+1<nx;k+=2){int ix0=std::max(0,(int)xs[k]),ix1=std::min(g_W-1,(int)xs[k+1]);for(int ix=ix0;ix<=ix1;ix++){float dx=ix+0.5f-gcx,dy=fy-gcy,t=sqrtf(dx*dx+dy*dy)*ig;C4 cl=evalTeardropGrad(s0,s1,s2,t);if(cl.a>=1.f)blendAdd(ix,iy,cl.r,cl.g,cl.b,cl.a);}}}
    }
    C4 _color(float l,float it,float e,bool s){
        C4 res; if(s){ if(config.theme==1) res=C4(255,255,255,l*config.overall_opacity*e); else if(config.theme==2) res=C4(50,150,255,l*config.overall_opacity*e); else res=C4(config.scroll_r, config.scroll_g, config.scroll_b, l*config.scroll_a*e); }
        else if(lightning_active){ res=C4(180,220,255,l*config.overall_opacity*e); }
        else if(config.theme==1){ res=C4(200+55*l,230+25*l,255,l*config.overall_opacity*e); } else if(config.theme==2){ res=C4(20,100+155*l,200+55*l,l*config.overall_opacity*e); } else if(config.theme==4){ 
            float total_cycle = config.gradient_speed + config.gradient_reverse_speed;
            float elapsed = 0; if(duration_start >= 0) elapsed = (float)(frame_count - duration_start);
            float t = std::fmod(elapsed, total_cycle);
            float virtual_t;
            if(t < config.gradient_speed) { virtual_t = t; }
            else { virtual_t = config.gradient_speed * (1.0f - (t - config.gradient_speed) / config.gradient_reverse_speed); }
            res = cmatrix_get(virtual_t, config.custom_gradient, 10, config.gradient_speed);
            res.a = l * config.overall_opacity * e;
        }
        else { float r,g,b; if(l>0.8f){r=255;g=220+35*(l-0.8f)*4.17f;b=150+105*(l-0.8f)*4.17f;}else if(l>0.6f){r=255;g=150+70*(l-0.6f)*3.33f;b=50+50*(l-0.6f)*1.67f;}else if(l>0.4f){r=230+25*(l-0.4f)*1.25f;g=80+70*(l-0.4f)*1.25f;b=25+25*(l-0.4f);}else if(l>0.2f){r=180+50*(l-0.2f)*2.5f;g=40+40*(l-0.2f)*2.f;b=25*(l-0.2f);}else{r=80+100*l;g=20*l;b=0;}
            float elapsed=0; if(duration_start>=0) elapsed=(frame_count-duration_start)/60.f;
            C4 base=cmatrix_get(elapsed, KEYFRAMES, 6, 60.0f); r=std::min(255.f,r*base.r/255.f);g=std::min(255.f,g*base.g/255.f);b=std::min(255.f,b*base.b/255.f); res=C4(r,g,b,l*config.overall_opacity*e); }
        if(color_override_t>0){res=c4lerp(res,color_override,color_override_t);} return c4clamp(res);
    }
    float _edge_fade(float x,float y){float d=std::min(std::min(x,(float)g_W-x),std::min(y,(float)g_H-y));return (d>=60.f)?1.f:powf(d/60.f,2.f);}
};

static bool g_enabled=1;
int main(){
    Display* d=XOpenDisplay(NULL);if(!d)return 1;int sn=DefaultScreen(d);Window r=RootWindow(d,sn);XVisualInfo vi;if(!XMatchVisualInfo(d,sn,32,TrueColor,&vi))return 1;
    g_W=DisplayWidth(d,sn);g_H=DisplayHeight(d,sn);g_buf.resize(g_W*g_H,0);XSetWindowAttributes at;at.colormap=XCreateColormap(d,r,vi.visual,AllocNone);at.background_pixel=0;at.border_pixel=0;at.override_redirect=True;
    Window w=XCreateWindow(d,r,0,0,g_W,g_H,0,vi.depth,InputOutput,vi.visual,CWColormap|CWBorderPixel|CWBackPixel|CWOverrideRedirect,&at);
    std::cout<<"CPU Dual-Core Renderer Enabled\n";
    XShapeCombineRectangles(d,w,ShapeInput,0,0,NULL,0,ShapeSet,YXBanded);Atom wt=XInternAtom(d,"_NET_WM_WINDOW_TYPE",0),tu=XInternAtom(d,"_NET_WM_WINDOW_TYPE_UTILITY",0);XChangeProperty(d,w,wt,XA_ATOM,32,PropModeReplace,(unsigned char*)&tu,1);XMapWindow(d,w);
    XImage* xi=XCreateImage(d,vi.visual,vi.depth,ZPixmap,0,(char*)g_buf.data(),g_W,g_H,32,0);GC gc=XCreateGC(d,w,0,NULL);
    int xo,xe,xer;if(!XQueryExtension(d,"XInputExtension",&xo,&xe,&xer))return 1;int ma=2,mi=0;XIQueryVersion(d,&ma,&mi);XIEventMask em;unsigned char md[XIMaskLen(XI_LASTEVENT)];memset(md,0,sizeof(md));em.deviceid=XIAllMasterDevices;em.mask_len=sizeof(md);em.mask=md;XISetMask(md,XI_RawButtonPress);XISetMask(md,XI_RawKeyPress);XISelectEvents(d,r,&em,1);
    KursorFlame fl;Window rr,cr;int rx,ry,wx,wy;unsigned int m;XQueryPointer(d,r,&rr,&cr,&rx,&ry,&wx,&wy,&m);fl.init(g_W,g_H,rx,ry);
    auto lu=std::chrono::steady_clock::now(),lc=lu;KeyCode ke=XKeysymToKeycode(d,XK_E),kq=XKeysymToKeycode(d,XK_Q);bool rn=1;
    while(rn){
        while(XPending(d)){XEvent ev;XNextEvent(d,&ev);if(XGetEventData(d,&ev.xcookie)){XGenericEventCookie* c=&ev.xcookie;if(c->extension==xo&&c->evtype==XI_RawKeyPress){XIRawEvent* ra=(XIRawEvent*)c->data;XQueryPointer(d,r,&rr,&cr,&rx,&ry,&wx,&wy,&m);if((m&ControlMask)&&(m&Mod1Mask)){if(ra->detail==ke){g_enabled=!g_enabled;if(g_enabled){XMapWindow(d,w);fl.config.load("kursor.conf");}else XUnmapWindow(d,w);}else if(ra->detail==kq)rn=0;}}else if(c->extension==xo&&c->evtype==XI_RawButtonPress){XIRawEvent* ra=(XIRawEvent*)c->data;if(ra->detail==4)fl.on_scroll(1);else if(ra->detail==5)fl.on_scroll(-1);}XFreeEventData(d,&ev.xcookie);}}
        if(g_enabled){auto n=std::chrono::steady_clock::now();if(std::chrono::duration_cast<std::chrono::milliseconds>(n-lc).count()>=8){XQueryPointer(d,r,&rr,&cr,&rx,&ry,&wx,&wy,&m);fl.on_move(rx,ry);fl.on_click((m&Button1Mask)||(m&Button3Mask));fl.cursor_tick();lc=n;}if(std::chrono::duration_cast<std::chrono::milliseconds>(n-lu).count()>=16){fl.update_tick();fl.render();XPutImage(d,w,gc,xi,0,0,0,0,g_W,g_H);XFlush(d);lu=n;}}else std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    XCloseDisplay(d);return 0;
}
