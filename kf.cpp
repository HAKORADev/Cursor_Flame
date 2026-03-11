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

namespace Cfg {
    constexpr int   EDGE_MARGIN             = 60;
    constexpr float FADE_POWER              = 2.0f;
    constexpr float EDGE_SOFT_THRESHOLD     = 0.05f;

    constexpr float PHYS_DRAG               = 0.98f;
    constexpr float PHYS_GRAVITY            = 0.02f;
    constexpr float PHYS_WOBBLE             = 0.3f;

    constexpr float VELOCITY_THRESHOLD      = 1.0f;
    constexpr float VELOCITY_SPREAD         = 0.5f;

    constexpr int   SPAWN_BASE              = 7;
    constexpr float SPAWN_OFFSET_X          = 15.0f;
    constexpr float SPAWN_OFFSET_Y          = 10.0f;
    constexpr float SPAWN_SPEED_MIN         = 2.0f;
    constexpr float SPAWN_SPEED_MAX         = 5.0f;

    constexpr float SPARK_MERGE_DIST        = 25.0f;
    constexpr float SPAWN_SCROLL_SENS       = 2.0f;
    constexpr float SPARK_DECAY             = 0.11f;

    constexpr int   SMOKE_MAX               = 30;
    constexpr float SMOKE_CHANCE            = 0.02f;
    constexpr float SMOKE_LIFE_THR          = 0.3f;
    constexpr float SMOKE_DECAY             = 0.01f;

    constexpr float INTERP_THRESH           = 20.0f;
    constexpr float INTERP_STEP             = 8.0f;

    constexpr float WIND_DECAY              = 0.95f;

    constexpr float BURNOUT_TOLERANCE       = 2.5f;
    constexpr float BURNOUT_SPEED_MIN       = 1.0f;
    constexpr float BURNOUT_SPEED_MAX       = 3.0f;

    constexpr float FIREBALL_THR            = 15.0f;
    constexpr float FIREBALL_INHERIT        = 0.6f;
    constexpr float FIREBALL_LIFE           = 1.5f;
    constexpr int   FIREBALL_COUNT          = 2;

    constexpr int   COLOR_ALPHA             = 220;

    constexpr float BURNOUT_PHASE_1_DURATION = 5000.0f;
    constexpr float BURNOUT_PHASE_2_DURATION = 10000.0f;
    constexpr float BURNOUT_PHASE_3_DURATION = 15000.0f;
    constexpr float BURNOUT_PHASE_4_DURATION = 5000.0f;
}

enum class State       { NORMAL, BURNOUT, FIREBALL };
enum class BurnoutPhase{ NONE, PHASE_1, PHASE_2, PHASE_3, PHASE_4 };

static std::mt19937 g_rng([]{ std::random_device rd; return rd(); }());

static inline float randf(float lo, float hi) {
    return std::uniform_real_distribution<float>(lo, hi)(g_rng);
}
static inline float randf01() {
    return std::uniform_real_distribution<float>(0.0f, 1.0f)(g_rng);
}

struct C4 {
    float r, g, b, a;
    C4(float r=0,float g=0,float b=0,float a=255):r(r),g(g),b(b),a(a){}
};
static C4 c4lerp(C4 a, C4 b, float t) {
    return {a.r+(b.r-a.r)*t, a.g+(b.g-a.g)*t, a.b+(b.b-a.b)*t, a.a+(b.a-a.a)*t};
}
static C4 c4clamp(C4 c) {
    return {std::min(255.f,std::max(0.f,c.r)), std::min(255.f,std::max(0.f,c.g)),
        std::min(255.f,std::max(0.f,c.b)), std::min(255.f,std::max(0.f,c.a))};
}
static C4 c4mul(C4 c, float s) { return {c.r*s,c.g*s,c.b*s,c.a*s}; }

struct KF { float t,r,g,b; };
static const KF KEYFRAMES[] = {
    { 0.f,  255, 80,  0   },
    { 2.f,  255, 120, 0   },
    { 4.f,  255, 180, 0   },
    { 6.f,  100, 150, 255 },
    {30.f,  150, 50,  200 },
    {60.f,  10,  10,  15  }
};
static constexpr int KF_N = 6;

static C4 cmatrix_get(float elapsed) {
    if (elapsed >= KEYFRAMES[KF_N-1].t)
        return {KEYFRAMES[KF_N-1].r, KEYFRAMES[KF_N-1].g, KEYFRAMES[KF_N-1].b, 220};
    for (int i=0; i<KF_N-1; i++) {
        if (elapsed < KEYFRAMES[i+1].t) {
            float f = (elapsed - KEYFRAMES[i].t) / (KEYFRAMES[i+1].t - KEYFRAMES[i].t);
            f = std::max(0.f, std::min(1.f, f));
            return { KEYFRAMES[i].r + (KEYFRAMES[i+1].r - KEYFRAMES[i].r)*f,
                KEYFRAMES[i].g + (KEYFRAMES[i+1].g - KEYFRAMES[i].g)*f,
                KEYFRAMES[i].b + (KEYFRAMES[i+1].b - KEYFRAMES[i].b)*f,
                220 };
        }
    }
    return {KEYFRAMES[KF_N-1].r, KEYFRAMES[KF_N-1].g, KEYFRAMES[KF_N-1].b, 220};
}

struct PMatrix {
    static constexpr int SIZE = 200;
    float x[SIZE],y[SIZE],vx[SIZE],vy[SIZE];
    float life[SIZE],max_life[SIZE],size_[SIZE];
    int   active[SIZE],is_scroll[SIZE];

    PMatrix() { memset(this,0,sizeof(*this)); }

    void spawn(int i,float x_,float y_,float vx_,float vy_,
               float life_,float sz,bool scroll=false) {
        x[i]=x_; y[i]=y_; vx[i]=vx_; vy[i]=vy_;
        life[i]=life_; max_life[i]=life_; size_[i]=sz;
        active[i]=1; is_scroll[i]=scroll?1:0;
    }
    void kill(int i)         { active[i]=0; }
    bool is_active(int i)    { return active[i]==1; }
    int  count() const       { int c=0; for(int i=0;i<SIZE;i++) c+=active[i]; return c; }
};

struct SparkPool {
    static constexpr int MAX = 50;
    float x[MAX],y[MAX],life[MAX];
    int   active[MAX];

    SparkPool() { memset(this,0,sizeof(*this)); }

    void spawn(int i,float cx,float cy) {
        x[i]=cx+randf(-30,30); y[i]=cy+randf(-25,35);
        life[i]=1.0f; active[i]=1;
    }
    void kill(int i) { active[i]=0; }
    void update(int i) {
        if (!active[i]) return;
        life[i] -= Cfg::SPARK_DECAY;
        x[i] += randf(-2,2) + sinf(life[i]*20)*2.5f;
        y[i] += randf(-3,1);
        if (life[i]<=0) kill(i);
    }
    int count() const { int c=0; for(int i=0;i<MAX;i++) c+=active[i]; return c; }
};

struct SmokePool {
    static constexpr int MAX = 30;
    float x[MAX],y[MAX],life[MAX],sz[MAX];
    int   active[MAX];

    SmokePool() { memset(this,0,sizeof(*this)); }

    void spawn(int i,float cx,float cy,float size_) {
        x[i]=cx; y[i]=cy; life[i]=1.0f; sz[i]=size_; active[i]=1;
    }
    void kill(int i) { active[i]=0; }
    void update(int i) {
        if (!active[i]) return;
        life[i] -= Cfg::SMOKE_DECAY;
        sz[i]   += 0.02f;
        y[i]    -= randf(0.5f,1.5f);
        x[i]    += randf(-0.5f,0.5f);
        if (life[i]<=0) kill(i);
    }
    int count() const { int c=0; for(int i=0;i<MAX;i++) c+=active[i]; return c; }
};

static int              g_W = 0, g_H = 0;
static std::vector<uint32_t> g_buf; // ARGB format

static void renderClear() { memset(g_buf.data(), 0, g_buf.size()*4); }

static inline void blendAdd(int px, int py, float r, float g, float b, float a) {
    if (px<0||px>=g_W||py<0||py>=g_H) return;
    int ia = (int)a; if (ia<=0) return;
    int pr = (int)r * ia / 255;
    int pg = (int)g * ia / 255;
    int pb = (int)b * ia / 255;
    
    uint32_t* d = &g_buf[py*g_W + px];
    uint8_t* p = (uint8_t*)d;
    p[0] = (uint8_t)std::min(255,(int)p[0]+pb);
    p[1] = (uint8_t)std::min(255,(int)p[1]+pg);
    p[2] = (uint8_t)std::min(255,(int)p[2]+pr);
    p[3] = (uint8_t)std::min(255,(int)p[3]+ia);
}

static C4 evalTeardropGrad(const C4& s0, const C4& s1, const C4& s2, float t) {
    if (t <= 0.0f) return s0;
    if (t <= 0.4f) return c4lerp(s0, s1, t / 0.4f);
    if (t <= 0.7f) return c4lerp(s1, s2, (t-0.4f)/0.3f);
    if (t <= 1.0f) return c4lerp(s2, C4(0,0,0,0), (t-0.7f)/0.3f);
    return C4(0,0,0,0);
}

static void drawTeardrop(const float pts[][2], float gcx, float gcy,
                         float grad_radius, const C4& color)
{
    C4 s0(color.r, color.g, color.b, std::min(255.f, color.a*1.2f));
    C4 s1 = color;
    C4 s2(color.r, color.g, color.b, color.a*0.3f);

    float minX=pts[0][0], maxX=pts[0][0], minY=pts[0][1], maxY=pts[0][1];
    for (int i=1; i<6; i++) {
        minX=std::min(minX,pts[i][0]); maxX=std::max(maxX,pts[i][0]);
        minY=std::min(minY,pts[i][1]); maxY=std::max(maxY,pts[i][1]);
    }
    int iy0 = std::max(0, (int)minY - 1);
    int iy1 = std::min(g_H-1, (int)maxY + 1);

    float inv_gr = grad_radius > 0.f ? 1.f/grad_radius : 0.f;

    for (int iy=iy0; iy<=iy1; iy++) {
        float fy = iy + 0.5f;
        float xs[8]; int nx=0;
        for (int i=0; i<6; i++) {
            int j = (i+1)%6;
            float y_a=pts[i][1], y_b=pts[j][1];
            float x_a=pts[i][0], x_b=pts[j][0];
            if ((fy>=y_a&&fy<y_b)||(fy>=y_b&&fy<y_a)) {
                float t=(fy-y_a)/(y_b-y_a);
                if (nx<8) xs[nx++]=x_a+t*(x_b-x_a);
            }
        }
        if (nx<2) continue;
        std::sort(xs, xs+nx);

        for (int k=0; k+1<nx; k+=2) {
            int ix0=std::max(0,(int)xs[k]);
            int ix1=std::min(g_W-1,(int)xs[k+1]);
            for (int ix=ix0; ix<=ix1; ix++) {
                float dx=ix+0.5f-gcx, dy=fy-gcy;
                float t=sqrtf(dx*dx+dy*dy)*inv_gr;
                C4 c=evalTeardropGrad(s0,s1,s2,t);
                if (c.a<1.f) continue;
                blendAdd(ix,iy,c.r,c.g,c.b,c.a);
            }
        }
    }
}

static void drawSpark(float x, float y, float life, float intensity) {
    float rc  = std::min(255.f, 255.f * intensity);
    float gc2 = std::min(255.f, 220.f + 35.f * life);
    float bc  = 180.f + 75.f * life;
    float a   = life * 255.f;
    float sz  = std::max(1.f, life * 8.f);
    float rad = sz * 2.f;

    int x0=std::max(0,(int)(x-rad)), x1=std::min(g_W-1,(int)(x+rad+1));
    int y0=std::max(0,(int)(y-rad)), y1=std::min(g_H-1,(int)(y+rad+1));

    for (int py=y0; py<=y1; py++) {
        float dy=py+0.5f-y;
        for (int px=x0; px<=x1; px++) {
            float dx=px+0.5f-x;
            float t=sqrtf(dx*dx+dy*dy)/rad;
            if (t>1.f) continue;
            float ca;
            if      (t<=0.5f) ca=a*1.3f+(a-a*1.3f)*(t/0.5f);
            else if (t<=0.8f) ca=a+(a*0.2f-a)*((t-0.5f)/0.3f);
            else              ca=a*0.2f*(1.f-(t-0.8f)/0.2f);
            ca=std::min(255.f, std::max(0.f, ca));
            blendAdd(px, py, rc, gc2, bc, ca);
        }
    }
}

static void drawSmoke(float sx, float sy, float life, float size) {
    float a = life * 60.f;
    int x0=std::max(0,(int)(sx-size)), x1=std::min(g_W-1,(int)(sx+size+1));
    int y0=std::max(0,(int)(sy-size)), y1=std::min(g_H-1,(int)(sy+size+1));
    float r2 = size*size;
    for (int py=y0; py<=y1; py++) {
        float dy=py+0.5f-sy;
        for (int px=x0; px<=x1; px++) {
            float dx=px+0.5f-sx;
            float d2=dx*dx+dy*dy; if(d2>r2) continue;
            float t=sqrtf(d2)/size;
            float ca=a*(1.f-t);
            blendAdd(px, py, 80, 80, 80, ca);
        }
    }
}

class CursorFlame {
public:
    PMatrix   p;
    SparkPool sparks;
    SmokePool smoke;

    float cx=0,cy=0, px=0,py=0, vx=0,vy=0, speed=0;
    int   screen_width=0, screen_height=0;

    State       state           = State::NORMAL;
    bool        mouse_down      = false;
    uint64_t    hold_start      = 0;
    float       hold_duration   = 0;
    float       max_hold_duration = 10000;

    float       wind_x=0, wind_y=0;
    int         wind_timer=0;

    bool        burnout_active     = false;
    float       burnout_transition = 0;
    BurnoutPhase burnout_phase     = BurnoutPhase::NONE;
    float       phase_start_time   = 0;
    int         burnout_cycle_count= 0;

    bool  lightning_active   = false;
    int   lightning_frame    = 0;
    int   lightning_max_frames = 8;
    float lightning_radius   = 0;

    C4    color_override     {0,0,0,0};
    float color_override_t   = 0;
    float color_decay        = 0;

    int   duration_start     = -1;
    int   frame_count        = 0;

    int   pidx=0, sidx=0, smidx=0;

    uint64_t start_time_ms;

    CursorFlame() {
        auto now = std::chrono::steady_clock::now();
        start_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    }

    uint64_t get_now_ms() {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    }

    void init(int sw, int sh, float init_x, float init_y) {
        screen_width=sw; screen_height=sh;
        cx=px=init_x; cy=py=init_y;
    }

    void on_move(float x, float y) { cx=x; cy=y; }

    void on_click(bool pressed) {
        if (pressed && !mouse_down) {
            hold_start = get_now_ms();
            _start_lightning_effect();
            color_override = C4(255,255,200,255);
            color_override_t = 1.0f;
            color_decay = 0.15f;
            burnout_cycle_count = 0;
            burnout_phase = BurnoutPhase::NONE;
            phase_start_time = 0;
        } else if (!pressed && mouse_down) {
            burnout_active     = false;
            burnout_transition = 0;
            state              = State::NORMAL;
            burnout_phase      = BurnoutPhase::NONE;
            phase_start_time   = 0;
        }
        mouse_down = pressed;
    }

    void on_scroll(int dy) {
        if (dy != 0) {
            float scroll_vy = -dy * Cfg::SPAWN_SCROLL_SENS;
            for (int i=0; i<8; i++)
                _spawn_scroll_particle(cx+randf(-30,30), cy+randf(-30,30), scroll_vy);
        }
    }

    void cursor_tick() {
        vx    = cx - px;
        vy    = cy - py;
        speed = sqrtf(vx*vx + vy*vy);
        px=cx; py=cy;
        _update_state();
    }

    void update_tick() {
        frame_count++;

        if (color_override_t > 0) {
            color_override_t -= color_decay;
            if (color_override_t <= 0) {
                color_override_t = 0;
                color_override = C4(0,0,0,0);
            }
        }

        if (wind_timer > 0) {
            wind_timer--;
            wind_x *= Cfg::WIND_DECAY;
            wind_y *= Cfg::WIND_DECAY;
        } else {
            wind_x = wind_y = 0;
        }

        if (p.count() > 0) {
            if (duration_start < 0) duration_start = frame_count;
        } else {
            duration_start = -1;
        }

        _interp_spawn();
        _spawn_mode();

        for (int i=0; i<PMatrix::SIZE; i++) {
            if (!p.is_active(i)) continue;
            p.life[i] -= p.max_life[i];
            p.x[i]    += p.vx[i] + wind_x;
            p.y[i]    += p.vy[i] + wind_y;

            float wobble = sinf(frame_count*0.1f + i) * Cfg::PHYS_WOBBLE;
            p.x[i]    += wobble * 0.3f;

            p.vx[i]   *= Cfg::PHYS_DRAG;
            p.vy[i]   *= Cfg::PHYS_DRAG;
            p.vy[i]   -= Cfg::PHYS_GRAVITY;

            if (p.life[i] <= 0) p.kill(i);
        }

        for (int i=0; i<SparkPool::MAX; i++) sparks.update(i);
        for (int i=0; i<SmokePool::MAX; i++) smoke.update(i);

        _merge_sparks();
        _emit_smoke();
    }

    void lightning_tick() {
        if (!lightning_active) return;
        lightning_frame++;
        lightning_radius = lightning_frame * 25.f;

        if (lightning_frame < 6) {
            int ring_count = 15 - lightning_frame*2;
            for (int i=0; i<ring_count; i++) {
                float angle  = randf(0, 6.283f);
                float radius = lightning_radius * randf(0.8f,1.2f);
                _spawn_particle(cx+cosf(angle)*radius, cy+sinf(angle)*radius,
                                0, 0, 1.2f, 1.0f);
            }
        }
        if (lightning_frame >= lightning_max_frames) {
            lightning_active = false;
            lightning_frame  = 0;
            lightning_radius = 0;
        }
    }

    void render() {
        renderClear();

        for (int i=0; i<SmokePool::MAX; i++) {
            if (!smoke.active[i]) continue;
            drawSmoke(smoke.x[i], smoke.y[i], smoke.life[i], smoke.sz[i]);
        }

        for (int i=0; i<PMatrix::SIZE; i++) {
            if (!p.is_active(i)) continue;
            float life = p.life[i];
            if (life <= 0) continue;
            float edge = _edge_fade(p.x[i], p.y[i]);
            if (edge < Cfg::EDGE_SOFT_THRESHOLD) continue;

            C4 color = _color(life, 1.0f, edge, p.is_scroll[i]!=0);
            float sz = std::max(4.f, p.size_[i]);
            float w  = sz * 1.4f;
            float h  = sz * 2.2f;
            float wob  = sinf(p.life[i]*10 + i) * 0.1f;
            float dist = 0.8f + life * 0.4f;

            _draw_teardrop_particle(p.x[i], p.y[i],
                                    w*(1+wob*0.3f), h,
                                    color, dist,
                                    p.vx[i], p.vy[i]);
        }

        for (int i=0; i<SparkPool::MAX; i++) {
            if (!sparks.active[i]) continue;
            drawSpark(sparks.x[i], sparks.y[i], sparks.life[i], 1.0f);
        }
    }

private:
    C4 _color_duration() {
        if (duration_start < 0) return C4(255,80,0,(float)Cfg::COLOR_ALPHA);
        float elapsed = (frame_count - duration_start) / 60.0f;
        return cmatrix_get(elapsed);
    }

    C4 _color(float life, float intensity, float edge_fade, bool is_scroll) {
        if (is_scroll)
            return C4(100, 200, 150, life * Cfg::COLOR_ALPHA * edge_fade);

        if (lightning_active) {
            float lp = (float)lightning_frame / lightning_max_frames;
            float l_r, l_g, l_b;
            if (lp < 0.2f) {
                float t=lp/0.2f;
                l_r=255*(1-t)+200*t; l_g=255*(1-t)+255*t; l_b=255*(1-t)+255*t;
            } else if (lp < 0.5f) {
                float t=(lp-0.2f)/0.3f;
                l_r=200*(1-t)+100*t; l_g=255*(1-t)+150*t; l_b=255;
            } else {
                float t=(lp-0.5f)/0.5f;
                l_r=100*(1-t)+180*t; l_g=150*(1-t)+100*t; l_b=255*(1-t)+200*t;
            }
            return C4(l_r, l_g, l_b, life * Cfg::COLOR_ALPHA * edge_fade);
        }

        C4 base = _color_duration();

        float r,g,b;
        if (life > 0.8f) {
            r=255; g=220+35*(life-0.8f)*4.17f; b=150+105*(life-0.8f)*4.17f;
        } else if (life > 0.6f) {
            r=255; g=150+70*(life-0.6f)*3.33f; b=50+50*(life-0.6f)*1.67f;
        } else if (life > 0.4f) {
            r=230+25*(life-0.4f)*1.25f; g=80+70*(life-0.4f)*1.25f; b=25+25*(life-0.4f);
        } else if (life > 0.2f) {
            r=180+50*(life-0.2f)*2.5f; g=40+40*(life-0.2f)*2.f; b=25*(life-0.2f);
        } else {
            r=80+100*life; g=20*life; b=0;
        }

        float iF = 0.8f + intensity * 0.2f;
        r = std::min(255.f, r * iF * base.r / 255.f);
        g = std::min(255.f, g * iF * base.g / 255.f);
        b = std::min(255.f, b * iF * base.b / 255.f);

        C4 base_m(r, g, b, life * Cfg::COLOR_ALPHA * edge_fade);

        if (color_override_t > 0 && color_override.a > 0) {
            C4 co = c4mul(color_override, color_override_t);
            base_m = c4lerp(base_m, co, color_override_t);
        }

        return c4clamp(base_m);
    }

    void _draw_teardrop_particle(float x, float y, float w, float h,
                                 const C4& color, float distortion,
                                 float pvx, float pvy)
    {
        float gcx = x;
        float gcy = y + h*0.15f;
        float s   = 1.8f * distortion;
        float hw  = w / 2.f;
        float hh  = h / 2.f;

        if (fabsf(pvx)>0.1f || fabsf(pvy)>0.1f) {
            float angle = atan2f(-pvy, -pvx);
            gcx += cosf(angle) * hw * 0.3f;
            gcy += sinf(angle) * hh * 0.3f;
        }

        float ty = gcy - hh * s;
        float by = gcy + hh * 0.5f;
        float tr = hw * 0.15f;

        float pts[7][2] = {
            {gcx,          ty                     },
            {gcx - hw + tr, gcy - hh*0.3f*s       },
            {gcx - hw,     by                     },
            {gcx,          by + hh*0.2f            },
            {gcx + hw,     by                     },
            {gcx + hw - tr, gcy - hh*0.3f*s       },
            {gcx,          ty                     }
        };

        drawTeardrop(pts, gcx, gcy, hw*1.2f, color);
    }

    float _edge_fade(float x, float y) {
        float d = std::min(std::min(x, (float)screen_width-x), std::min(y, (float)screen_height-y));
        if (d >= Cfg::EDGE_MARGIN) return 1.f;
        return powf(d / Cfg::EDGE_MARGIN, Cfg::FADE_POWER);
    }

    float _intensity() { return 0.6f + std::min(0.9f, speed/25.f); }

    float _get_spawn_angle(float pvx, float pvy) {
        if (sqrtf(pvx*pvx+pvy*pvy) < Cfg::VELOCITY_THRESHOLD)
            return randf(-2.199f, -0.942f);
        return atan2f(-pvy,-pvx) + randf(-Cfg::VELOCITY_SPREAD, Cfg::VELOCITY_SPREAD);
    }

    void _spawn_particle(float x, float y, float ivx, float ivy,
                         float intensity, float life_mult, bool is_scroll=false) {
        int idx = pidx;
        pidx = (pidx+1) % PMatrix::SIZE;
        float angle = _get_spawn_angle(ivx, ivy);
        float sp    = randf(Cfg::SPAWN_SPEED_MIN, Cfg::SPAWN_SPEED_MAX) * intensity;
        float pvx   = cosf(angle)*sp*randf(0.3f,0.8f);
        float pvy   = sinf(angle)*sp;
        float sz    = randf(8,20)*intensity;
        float life  = randf(0.015f,0.035f)*life_mult;
        p.spawn(idx, x,y, pvx,pvy, 1.0f, sz, is_scroll);
        p.max_life[idx] = life;
    }

    void _spawn_scroll_particle(float x, float y, float scroll_vy) {
        int idx = pidx;
        pidx = (pidx+1) % PMatrix::SIZE;
        float pvx  = randf(-1,1)*fabsf(scroll_vy);
        float pvy  = scroll_vy + randf(-0.5f,0.5f);
        float sz   = randf(10,18);
        float life = randf(0.02f,0.04f);
        p.spawn(idx, x,y, pvx,pvy, 1.0f, sz, true);
        p.max_life[idx] = life;
    }

    void _spawn_spark(float x, float y) {
        int idx=sidx; sidx=(sidx+1)%SparkPool::MAX;
        sparks.spawn(idx, x, y);
    }

    void _spawn_smoke(float x, float y) {
        int idx=smidx; smidx=(smidx+1)%SmokePool::MAX;
        smoke.spawn(idx, x, y, randf(10,15));
    }

    void _interp_spawn() {
        if (speed < Cfg::INTERP_THRESH) return;
        int steps = std::min((int)(speed/Cfg::INTERP_STEP), 50);
        for (int i=0; i<steps; i++) {
            float t=(i+1.f)/(steps+1.f);
            float sx=cx-vx*t, sy=cy-vy*t;
            if (sx>=0&&sx<=screen_width&&sy>=0&&sy<=screen_height)
                _spawn_particle(sx,sy, vx,vy, _intensity(), 1.f);
        }
    }

    void _spawn_mode() {
        switch (state) {
            case State::NORMAL:   _spawn_normal();   break;
            case State::BURNOUT:  _spawn_burnout();  break;
            case State::FIREBALL: _spawn_fireball(); break;
        }
    }

    void _spawn_normal() {
        int count=(int)(Cfg::SPAWN_BASE * std::min(2.5f,speed/12.f) * _intensity());
        for (int i=0; i<count; i++) {
            if (p.count()>=PMatrix::SIZE) break;
            _spawn_particle(cx+randf(-Cfg::SPAWN_OFFSET_X,Cfg::SPAWN_OFFSET_X),
                            cy+randf(-Cfg::SPAWN_OFFSET_Y,Cfg::SPAWN_OFFSET_Y),
                            vx,vy, _intensity(), 1.f);
        }
    }

    void _spawn_burnout() {
        if (burnout_phase == BurnoutPhase::NONE) {
            float hp = hold_duration / max_hold_duration;
            float sm = 1.f+hp*2.f, rm = 0.5f+hp*1.5f;
            int count=(int)(Cfg::SPAWN_BASE*2*rm);
            for (int i=0;i<count;i++) {
                if (p.count()>=PMatrix::SIZE) break;
                float a=randf(0,6.283f);
                float s=randf(Cfg::BURNOUT_SPEED_MIN,Cfg::BURNOUT_SPEED_MAX)*sm;
                _spawn_particle(cx+randf(-5,5), cy+randf(-5,5),
                                cosf(a)*s, sinf(a)*s, _intensity()*1.2f, 0.5f);
            }
        } else if (burnout_phase == BurnoutPhase::PHASE_1) {
            float pp=(hold_duration-phase_start_time)/Cfg::BURNOUT_PHASE_2_DURATION;
            float sa=1.f+pp*1.f, smm=1.f+pp*0.4f;
            int count=(int)(Cfg::SPAWN_BASE*3);
            for (int i=0;i<count;i++) {
                if (p.count()>=PMatrix::SIZE) break;
                float a=randf(0,6.283f);
                float s=randf(Cfg::BURNOUT_SPEED_MIN*2,Cfg::BURNOUT_SPEED_MAX*2)*sa;
                float x=cx+randf(-8,8), y=cy+randf(-8,8);
                _spawn_particle(x,y, cosf(a)*s,sinf(a)*s, _intensity()*1.5f, 0.3f);
                if (randf01()<Cfg::SMOKE_CHANCE*smm) _spawn_smoke(x,y);
            }
        } else if (burnout_phase == BurnoutPhase::PHASE_2) {
            int count=(int)(Cfg::SPAWN_BASE*4);
            for (int i=0;i<count;i++) {
                if (p.count()>=PMatrix::SIZE) break;
                float a=randf(0,6.283f);
                float s=randf(Cfg::BURNOUT_SPEED_MIN*3,Cfg::BURNOUT_SPEED_MAX*3)*2.f;
                float x=cx+randf(-12,12), y=cy+randf(-12,12);
                _spawn_particle(x,y, cosf(a)*s,sinf(a)*s, _intensity()*2.f, 0.2f);
                if (randf01()<Cfg::SMOKE_CHANCE*1.4f) _spawn_smoke(x,y);
            }
        } else if (burnout_phase == BurnoutPhase::PHASE_3) {
            for (int i=0;i<3;i++)
                _spawn_smoke(cx+randf(-20,20), cy+randf(-20,20));
        } else if (burnout_phase == BurnoutPhase::PHASE_4) {
            burnout_phase    = BurnoutPhase::NONE;
            phase_start_time = hold_duration;
        }
    }

    void _spawn_fireball() {
        int count = Cfg::SPAWN_BASE * Cfg::FIREBALL_COUNT;
        float ma  = (fabsf(vx)>0.1f||fabsf(vy)>0.1f) ? atan2f(vy,vx) : -1.571f;
        for (int i=0;i<count;i++) {
            if (p.count()>=PMatrix::SIZE) break;
            float a=ma+3.141f+randf(-0.5f,0.5f);
            float s=randf(2,5)*_intensity();
            _spawn_particle(cx+randf(-Cfg::FIREBALL_THR,Cfg::FIREBALL_THR),
                            cy+randf(-Cfg::FIREBALL_THR,Cfg::FIREBALL_THR),
                            cosf(a)*s+vx*Cfg::FIREBALL_INHERIT,
                            sinf(a)*s+vy*Cfg::FIREBALL_INHERIT,
                            _intensity()*1.3f, 1.f/Cfg::FIREBALL_LIFE);
        }
    }

    void _update_state() {
        uint64_t now = get_now_ms();

        if (mouse_down) {
            hold_duration = std::min(max_hold_duration, (float)(now - hold_start));

            bool is_moving = fabsf(vx)>Cfg::BURNOUT_TOLERANCE ||
            fabsf(vy)>Cfg::BURNOUT_TOLERANCE;

            if (speed > Cfg::FIREBALL_THR) {
                state = State::FIREBALL;
                burnout_phase = BurnoutPhase::NONE;
            } else if (is_moving) {
                state = State::NORMAL;
                burnout_phase = BurnoutPhase::NONE;
            } else {
                state = State::BURNOUT;
                _update_burnout_phase();
            }
        } else {
            hold_duration = 0;
            state         = State::NORMAL;
            burnout_phase = BurnoutPhase::NONE;
        }
    }

    void _update_burnout_phase() {
        if (phase_start_time == 0) phase_start_time = hold_duration;
        float pd = hold_duration - phase_start_time;

        if (burnout_cycle_count < 5) {
            if (pd >= Cfg::BURNOUT_PHASE_1_DURATION) {
                burnout_phase = BurnoutPhase::PHASE_1;
                burnout_cycle_count++;
                phase_start_time = hold_duration;
            } else {
                burnout_phase = BurnoutPhase::NONE;
            }
        } else {
            switch (burnout_phase) {
                case BurnoutPhase::NONE:
                    burnout_phase=BurnoutPhase::PHASE_1; phase_start_time=hold_duration; break;
                case BurnoutPhase::PHASE_1:
                    if (pd>=Cfg::BURNOUT_PHASE_2_DURATION)
                    {burnout_phase=BurnoutPhase::PHASE_2; phase_start_time=hold_duration;} break;
                case BurnoutPhase::PHASE_2:
                    if (pd>=Cfg::BURNOUT_PHASE_3_DURATION)
                    {burnout_phase=BurnoutPhase::PHASE_3; phase_start_time=hold_duration;} break;
                case BurnoutPhase::PHASE_3:
                    if (pd>=Cfg::BURNOUT_PHASE_4_DURATION)
                    {burnout_phase=BurnoutPhase::PHASE_4; phase_start_time=hold_duration;
                        burnout_cycle_count=0;} break;
                default: break;
            }
        }
    }

    void _merge_sparks() {
        for (int si=0;si<SparkPool::MAX;si++) {
            if (!sparks.active[si]) continue;
            float sx=sparks.x[si], sy=sparks.y[si];
            for (int pi=0;pi<PMatrix::SIZE;pi++) {
                if (!p.is_active(pi)) continue;
                float dx=sx-p.x[pi], dy=sy-p.y[pi];
                if (dx*dx+dy*dy < Cfg::SPARK_MERGE_DIST*Cfg::SPARK_MERGE_DIST) {
                    p.life[pi]  = std::min(1.f, p.life[pi]+0.15f);
                    p.size_[pi] = std::min(p.max_life[pi]*1.5f, p.size_[pi]*1.2f);
                    sparks.kill(si); break;
                }
            }
        }
    }

    void _emit_smoke() {
        if (smoke.count()>=SmokePool::MAX) return;
        for (int i=0;i<PMatrix::SIZE;i++) {
            if (!p.is_active(i)) continue;
            if (p.life[i]<Cfg::SMOKE_LIFE_THR && randf01()<Cfg::SMOKE_CHANCE) {
                float sc=Cfg::SMOKE_CHANCE;
                if (state==State::BURNOUT &&
                    (burnout_phase==BurnoutPhase::PHASE_1 ||
                    burnout_phase==BurnoutPhase::PHASE_2)) sc*=1.4f;
                if (randf01()<sc) _spawn_smoke(p.x[i],p.y[i]);
            }
        }
    }

    void _start_lightning_effect() {
        lightning_active=true; lightning_frame=0; lightning_radius=0;
        for (int i=0;i<20;i++) {
            float a=randf(0,6.283f), s=randf(8,12);
            _spawn_particle(cx,cy, cosf(a)*s,sinf(a)*s, 1.5f, 2.f);
        }
        for (int i=0;i<15;i++) _spawn_spark(cx,cy);
        for (int i=0; i<8; i++) _spawn_smoke(cx,cy);
    }
};

static bool g_enabled = true;

int main() {
    Display* display = XOpenDisplay(NULL);
    if (!display) {
        std::cerr << "Cannot open display" << std::endl;
        return 1;
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    XVisualInfo vinfo;
    if (!XMatchVisualInfo(display, screen, 32, TrueColor, &vinfo)) {
        std::cerr << "No 32-bit visual found" << std::endl;
        return 1;
    }

    g_W = DisplayWidth(display, screen);
    g_H = DisplayHeight(display, screen);
    g_buf.resize((size_t)g_W * g_H, 0);

    XSetWindowAttributes attrs;
    attrs.colormap = XCreateColormap(display, root, vinfo.visual, AllocNone);
    attrs.background_pixel = 0;
    attrs.border_pixel = 0;
    attrs.override_redirect = True; 

    Window window = XCreateWindow(display, root, 0, 0, g_W, g_H, 0,
                                  vinfo.depth, InputOutput, vinfo.visual,
                                  CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect, &attrs);

    XShapeCombineRectangles(display, window, ShapeInput, 0, 0, NULL, 0, ShapeSet, YXBanded);

    Atom wm_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom type_utility = XInternAtom(display, "_NET_WM_WINDOW_TYPE_UTILITY", False);
    XChangeProperty(display, window, wm_type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&type_utility, 1);

    XMapWindow(display, window);

    XImage* ximage = XCreateImage(display, vinfo.visual, vinfo.depth, ZPixmap, 0,
                                  (char*)g_buf.data(), g_W, g_H, 32, 0);

    GC gc = XCreateGC(display, window, 0, NULL);

    // XInput2 Initialization
    int xi_opcode, xi_event, xi_error;
    if (!XQueryExtension(display, "XInputExtension", &xi_opcode, &xi_event, &xi_error)) {
        std::cerr << "XInput extension not available" << std::endl;
        return 1;
    }

    int major = 2, minor = 0;
    if (XIQueryVersion(display, &major, &minor) == BadRequest) {
        std::cerr << "XI2 not supported" << std::endl;
        return 1;
    }

    XIEventMask evmask;
    unsigned char mask_data[XIMaskLen(XI_LASTEVENT)];
    memset(mask_data, 0, sizeof(mask_data));
    evmask.deviceid = XIAllMasterDevices;
    evmask.mask_len = sizeof(mask_data);
    evmask.mask = mask_data;
    XISetMask(mask_data, XI_RawButtonPress);
    XISetMask(mask_data, XI_RawKeyPress);

    XISelectEvents(display, root, &evmask, 1);

    CursorFlame flame;
    Window root_ret, child_ret;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;
    XQueryPointer(display, root, &root_ret, &child_ret, &root_x, &root_y, &win_x, &win_y, &mask);
    flame.init(g_W, g_H, (float)root_x, (float)root_y);

    auto last_update = std::chrono::steady_clock::now();
    auto last_cursor_tick = std::chrono::steady_clock::now();
    auto last_lightning_tick = std::chrono::steady_clock::now();

    KeyCode key_e = XKeysymToKeycode(display, XK_E);
    KeyCode key_q = XKeysymToKeycode(display, XK_Q);

    bool running = true;
    while (running) {
        while (XPending(display)) {
            XEvent ev;
            XNextEvent(display, &ev);

            if (XGetEventData(display, &ev.xcookie)) {
                XGenericEventCookie* cookie = &ev.xcookie;
                if (cookie->extension == xi_opcode && cookie->evtype == XI_RawKeyPress) {
                    XIRawEvent* raw = (XIRawEvent*)cookie->data;
                    // Check modifiers manually or via XQueryPointer
                    XQueryPointer(display, root, &root_ret, &child_ret, &root_x, &root_y, &win_x, &win_y, &mask);
                    bool ctrl = (mask & ControlMask);
                    bool alt = (mask & Mod1Mask);

                    if (ctrl && alt) {
                        if (raw->detail == key_e) {
                            g_enabled = !g_enabled;
                            if (g_enabled) XMapWindow(display, window);
                            else XUnmapWindow(display, window);
                        } else if (raw->detail == key_q) {
                            running = false;
                        }
                    }
                } else if (cookie->extension == xi_opcode && cookie->evtype == XI_RawButtonPress) {
                    XIRawEvent* raw = (XIRawEvent*)cookie->data;
                    if (raw->detail == 4) flame.on_scroll(1);
                    else if (raw->detail == 5) flame.on_scroll(-1);
                }
                XFreeEventData(display, &ev.xcookie);
            }
        }

        if (g_enabled) {
            auto now = std::chrono::steady_clock::now();
            
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_cursor_tick).count() >= 8) {
                XQueryPointer(display, root, &root_ret, &child_ret, &root_x, &root_y, &win_x, &win_y, &mask);
                flame.on_move((float)root_x, (float)root_y);
                flame.on_click((mask & Button1Mask) || (mask & Button3Mask));
                flame.cursor_tick();
                last_cursor_tick = now;
            }

            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count() >= 16) {
                flame.update_tick();
                flame.render();
                XPutImage(display, window, gc, ximage, 0, 0, 0, 0, g_W, g_H);
                XFlush(display);
                last_update = now;
            }

            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_lightning_tick).count() >= 16) {
                flame.lightning_tick();
                last_lightning_tick = now;
            }
        } else {
            // Even if disabled, we should sleep to avoid 100% CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    XCloseDisplay(display);
    return 0;
}

