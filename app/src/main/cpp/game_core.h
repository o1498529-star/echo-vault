#pragma once
// Platform-independent game core shared by the Windows and Android builds.
// KEEP pixel_game/game_core.h AND pixel_android/app/src/main/cpp/game_core.h IDENTICAL (see sync_core.sh).
//
// The platform file must, BEFORE including this header, provide <cstdint>-style
// VK_* key codes (VK_SHIFT, VK_ESCAPE, VK_SPACE, VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN, VK_RETURN),
// and may define EV_TOUCH for touch-oriented texts.
// It must, AFTER including this header, define:
//     static void sound(int id);     // play effect 0..4 (the core already checks `muted`)
//     static void apply_mute();      // react to `muted` changing
// and drive the core with: step() once per 1/60 s, draw() to fill `vertices`.
#include <stdint.h>
#include <math.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

// Pixel art is emitted as colored rectangles and rasterized by OpenGL.
constexpr int W = 320, H = 240, TW = 20, TH = 15, TILE = 16, MAX_HP = 5;
constexpr float STEP = 1.0f / 60.0f;
struct GpuVertex { float x,y; uint8_t r,g,b,a; };
static_assert(sizeof(GpuVertex)==12,"Unexpected GPU vertex layout");
// A full frame uses well under 3000 rectangles; 6144 leaves a wide safety margin.
constexpr int MAX_RECTS=6144, MAX_VERTICES=MAX_RECTS*6;
static GpuVertex vertices[MAX_VERTICES];
static int vertex_count=0, cam_x=0, cam_y=0;
static bool keys[256], pressed[256];
static uint32_t random_state = 0x17ac091u;
static int frame_number = 0, shake = 0;
static bool muted=false;
static void sound(int id);   // provided by the platform
static void apply_mute();    // provided by the platform

static unsigned rnd() { random_state ^= random_state << 13; random_state ^= random_state >> 17; random_state ^= random_state << 5; return random_state; }
static uint32_t color(int r, int g, int b) { return (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b); }
static void rect(int x, int y, int w, int h, uint32_t c) {
    x+=cam_x; y+=cam_y;
    int x0=std::max(0,x),y0=std::max(0,y),x1=std::min(W,x+w),y1=std::min(H,y+h);
    if(x0>=x1 || y0>=y1 || vertex_count+6>MAX_VERTICES) return;
    float left=float(x0)*2/W-1, right=float(x1)*2/W-1, top=1-float(y0)*2/H, bottom=1-float(y1)*2/H;
    uint8_t r=uint8_t(c>>16),g=uint8_t(c>>8),b=uint8_t(c);
    GpuVertex a{left,top,r,g,b,255}, q{right,top,r,g,b,255}, d{left,bottom,r,g,b,255}, e{right,bottom,r,g,b,255};
    vertices[vertex_count++]=a;vertices[vertex_count++]=d;vertices[vertex_count++]=q;
    vertices[vertex_count++]=q;vertices[vertex_count++]=d;vertices[vertex_count++]=e;
}
static void pixel(int x,int y,uint32_t c) {rect(x,y,1,1,c);}
static void box(int x, int y, int w, int h, uint32_t c) { rect(x,y,w,1,c); rect(x,y+h-1,w,1,c); rect(x,y,1,h,c); rect(x+w-1,y,1,h,c); }

// 5x7 bitmap alphabet, stored in ROM-like static data.
static const char* glyph(char c) {
    if(c>='a' && c<='z') c=char(c-32);
    switch(c) {
    case 'A':return "01110/10001/10001/11111/10001/10001/10001";
    case 'B':return "11110/10001/10001/11110/10001/10001/11110";
    case 'C':return "01111/10000/10000/10000/10000/10000/01111";
    case 'D':return "11110/10001/10001/10001/10001/10001/11110";
    case 'E':return "11111/10000/10000/11110/10000/10000/11111";
    case 'F':return "11111/10000/10000/11110/10000/10000/10000";
    case 'G':return "01111/10000/10000/10111/10001/10001/01111";
    case 'H':return "10001/10001/10001/11111/10001/10001/10001";
    case 'I':return "11111/00100/00100/00100/00100/00100/11111";
    case 'J':return "00111/00010/00010/00010/10010/10010/01100";
    case 'K':return "10001/10010/10100/11000/10100/10010/10001";
    case 'L':return "10000/10000/10000/10000/10000/10000/11111";
    case 'M':return "10001/11011/10101/10101/10001/10001/10001";
    case 'N':return "10001/11001/10101/10011/10001/10001/10001";
    case 'O':return "01110/10001/10001/10001/10001/10001/01110";
    case 'P':return "11110/10001/10001/11110/10000/10000/10000";
    case 'Q':return "01110/10001/10001/10001/10101/10010/01101";
    case 'R':return "11110/10001/10001/11110/10100/10010/10001";
    case 'S':return "01111/10000/10000/01110/00001/00001/11110";
    case 'T':return "11111/00100/00100/00100/00100/00100/00100";
    case 'U':return "10001/10001/10001/10001/10001/10001/01110";
    case 'V':return "10001/10001/10001/10001/10001/01010/00100";
    case 'W':return "10001/10001/10001/10101/10101/10101/01010";
    case 'X':return "10001/10001/01010/00100/01010/10001/10001";
    case 'Y':return "10001/10001/01010/00100/00100/00100/00100";
    case 'Z':return "11111/00001/00010/00100/01000/10000/11111";
    case '0':return "01110/10001/10011/10101/11001/10001/01110";
    case '1':return "00100/01100/00100/00100/00100/00100/01110";
    case '2':return "01110/10001/00001/00010/00100/01000/11111";
    case '3':return "11110/00001/00001/01110/00001/00001/11110";
    case '4':return "00010/00110/01010/10010/11111/00010/00010";
    case '5':return "11111/10000/10000/11110/00001/00001/11110";
    case '6':return "01110/10000/10000/11110/10001/10001/01110";
    case '7':return "11111/00001/00010/00100/01000/01000/01000";
    case '8':return "01110/10001/10001/01110/10001/10001/01110";
    case '9':return "01110/10001/10001/01111/00001/00001/01110";
    case ':':return "00000/00100/00100/00000/00100/00100/00000";
    case '.':return "00000/00000/00000/00000/00000/00110/00110";
    case ',':return "00000/00000/00000/00000/00110/00100/01000";
    case '/':return "00001/00010/00010/00100/01000/01000/10000";
    case '-':return "00000/00000/00000/11111/00000/00000/00000";
    case '+':return "00000/00100/00100/11111/00100/00100/00000";
    case '!':return "00100/00100/00100/00100/00100/00000/00100";
    case '?':return "01110/10001/00001/00110/00100/00000/00100";
    case '>':return "10000/01000/00100/00010/00100/01000/10000";
    default:return "00000/00000/00000/00000/00000/00000/00000";
    }
}
// Horizontal runs of lit pixels are merged into one rectangle: ~40% fewer vertices than per-pixel quads.
static void text(int x, int y, const char* s, uint32_t c, int scale=1) {
    for (; *s; ++s, x += 6 * scale) {
        const char* g = glyph(*s);
        for (int row = 0; row < 7; ++row) {
            int col=0;
            while(col<5) {
                if(g[row*6+col]=='1') { int start=col; while(col<5 && g[row*6+col]=='1') col++; rect(x+start*scale,y+row*scale,(col-start)*scale,scale,c); }
                else col++;
            }
        }
    }
}
static void centered(int y, const char* s, uint32_t c, int scale=1) { text((W - int(strlen(s))*6*scale)/2, y, s, c, scale); }

#include "campaign.h"
static char world[TH][TW+1];
static int distance_map[TH][TW];
struct Bullet { float x,y,vx,vy; int life; bool active; };
struct Enemy { float x,y; int hp, hurt, delay; bool active; int type, cooldown; };
struct Particle { float x,y,vx,vy; int life; uint32_t c; };
struct Crystal { int x,y; bool collected; };
static Bullet bullets[24], enemy_bullets[32]; static Enemy enemies[12]; static Particle particles[80];
static Crystal crystals[5];
static float px,py,fx,fy; static int health, collected, shots, invulnerable, spawn_timer, score, elapsed;
static int dash_cooldown=0, dash_frames=0, boss_health=0, boss_cooldown=0, boss_hurt=0, pickup_x=0, pickup_y=0, pickup_life=0;
static int combo=0, combo_timer=0;
static bool boss_defeated=false;
static int weapon=0, upgrade=0, damage_upgrade=0, speed_upgrade=0, dash_upgrade=0, sword_timer=0, sword_cooldown=0, boss_max_health=24;
static int level=0, goal=3, level_start_score=0, level_start_elapsed=0, level_start_health=MAX_HP;
static bool is_boss_stage() {return campaign[level].boss>=0;}
enum Scene { TITLE, PLAY, DEAD, STAGE_CLEAR, WIN, PAUSED, HELP, SETTINGS }; static Scene scene = TITLE;
static int pause_choice=0, help_page=0, settings_choice=0, difficulty=1, player_color=0, starting_weapon=0;
static Scene settings_return=TITLE;
struct Difficulty { const char* name; float player_speed, enemy_speed, spawn_factor; int enemy_limit; };
static const Difficulty difficulties[]={{"EASY",1.25f,0.70f,1.50f,3},{"NORMAL",1.15f,1.00f,1.00f,5},{"HARD",1.05f,1.25f,0.78f,8},{"VERY HARD",0.98f,1.48f,0.60f,12}};
static const char* color_names[]={"CYAN","GOLD","PINK","GREEN"};
static const uint32_t player_colors[]={0x78cde7,0xf2c45f,0xed92bf,0x82dc9a};

// ---- Saved data: settings and best results per difficulty (13 ints, platform stores them) ----
constexpr int SAVE_INTS=13, SAVE_MAGIC=0x45560001;
static int best_score[4]={0,0,0,0}, best_stage[4]={0,0,0,0};
static bool save_dirty=false, new_best=false;
static void save_pack(int* out) {
    out[0]=SAVE_MAGIC; out[1]=difficulty; out[2]=player_color; out[3]=starting_weapon; out[4]=muted?1:0;
    for(int i=0;i<4;i++) {out[5+i]=best_score[i];out[9+i]=best_stage[i];}
}
static bool save_unpack(const int* in) {
    if(in[0]!=SAVE_MAGIC) return false;
    difficulty=std::min(3,std::max(0,in[1])); player_color=std::min(3,std::max(0,in[2])); starting_weapon=in[3]?1:0; muted=in[4]!=0;
    for(int i=0;i<4;i++) {best_score[i]=std::max(0,in[5+i]);best_stage[i]=std::min(LEVELS,std::max(0,in[9+i]));}
    weapon=starting_weapon; apply_mute(); return true;
}
static void record_progress() {
    if(score>best_score[difficulty]) {best_score[difficulty]=score;new_best=true;save_dirty=true;}
    if(level+1>best_stage[difficulty]) {best_stage[difficulty]=level+1;save_dirty=true;}
}

static void change_setting(int delta) {
    if(settings_choice==0) difficulty=(difficulty+delta+4)%4;
    else if(settings_choice==1) player_color=(player_color+delta+4)%4;
    else if(settings_choice==2) starting_weapon=1-starting_weapon;
    else if(settings_choice==3) {muted=!muted;apply_mute();}
    save_dirty=true;
}
static void toggle_mute() { muted=!muted; apply_mute(); if(!muted) sound(3); save_dirty=true; }
static void open_settings(Scene back) { settings_return=back; settings_choice=0; scene=SETTINGS; }
static void update_settings() {
    if(pressed[VK_UP]||pressed['W']) settings_choice=(settings_choice+4)%5;
    if(pressed[VK_DOWN]||pressed['S']) settings_choice=(settings_choice+1)%5;
    if(pressed[VK_LEFT]||pressed['A']) change_setting(-1);
    if(pressed[VK_RIGHT]||pressed['D']) change_setting(1);
    if(pressed['C']||pressed[VK_ESCAPE]||pressed['P']||(settings_choice==4 && pressed[VK_RETURN])) scene=settings_return;
}
static bool blocked(float x,float y,float r) {
    if(x-r<16 || y-r<16 || x+r>W-16 || y+r>H-16) return true;
    int minx=std::max(1,int((x-r-7)/TILE)), maxx=std::min(TW-2,int((x+r+7)/TILE));
    int miny=std::max(1,int((y-r-7)/TILE)), maxy=std::min(TH-2,int((y+r+7)/TILE));
    for(int ty=miny;ty<=maxy;ty++) for(int tx=minx;tx<=maxx;tx++) if(world[ty][tx]=='#') {
        float dx=x-(tx*TILE+TILE/2),dy=y-(ty*TILE+TILE/2),half=r+7.0f;
        if(fabsf(dx)<half && fabsf(dy)<half) return true;
    }
    return false;
}
static bool wall(float x,float y) {return blocked(x,y,0);}
static float length(float x,float y) { return sqrtf(x*x+y*y); }
static void move_entity(float& x, float& y, float dx,float dy,float radius) {
    if (!blocked(x+dx,y,radius)) x+=dx;
    if (!blocked(x,y+dy,radius)) y+=dy;
}
static void burst(float x,float y,uint32_t c,int count) {
    for (int n=0;n<count;n++) for (auto& p:particles) if(p.life<=0) {
        float a=float(rnd()%628)/100.0f, speed=0.3f+float(rnd()%150)/100.0f;
        p={x,y,cosf(a)*speed,sinf(a)*speed,12+int(rnd()%15),c}; break;
    }
}
static void make_world(int index) {
    for(int y=0;y<TH;y++) for(int x=0;x<TW;x++) {
        bool edge=x==0 || x==TW-1 || y==0 || y==TH-1;
        // Odd rows and columns are connected corridors; wall patterns vary by stage.
        bool pillar=(x%2==0 && y%2==0 && ((x*7+y*11+index*5)%7 < 5));
        world[y][x]=(edge||pillar)?'#':'.';
    }
    for(int y=0;y<TH;y++) world[y][TW]='\0';
}
static void begin_level() {
    memset(bullets,0,sizeof(bullets)); memset(enemy_bullets,0,sizeof(enemy_bullets)); memset(enemies,0,sizeof(enemies)); memset(particles,0,sizeof(particles));
    make_world(campaign[level].seed);
    // Odd tile coordinates are always connected by the corridor grid.
    goal=std::min(5,3+level/4);
    for(int i=0;i<5;i++) {int cell=(campaign[level].seed*11+i*13+8)%62+1;int tx=(cell%9)*2+1,ty=(cell/9)*2+1;crystals[i]={tx*16+8,ty*16+8,i>=goal};}
    px=24; py=24; fx=1; fy=0; collected=0; shots=0; invulnerable=0; spawn_timer=100; shake=0; combo=0; combo_timer=0; boss_hurt=0; new_best=false;
    dash_cooldown=0; dash_frames=0; sword_timer=0; sword_cooldown=0; boss_max_health=24+(level/6)*8; boss_health=boss_max_health; boss_cooldown=100; boss_defeated=false; pickup_life=0;
    level_start_score=score; level_start_elapsed=elapsed; level_start_health=health;
}
static void reset_game() {
    level=0; health=MAX_HP; score=0; elapsed=0; weapon=starting_weapon; upgrade=0; damage_upgrade=0; speed_upgrade=0; dash_upgrade=0; begin_level();
}
static void retry_level() {
    score=level_start_score; elapsed=level_start_elapsed; health=level_start_health; begin_level();
}
static void update_distance() {
    int qx[TW*TH],qy[TW*TH],head=0,tail=0;
    for(int y=0;y<TH;y++) for(int x=0;x<TW;x++) distance_map[y][x]=-1;
    int sx=int(px)/16,sy=int(py)/16;
    if(sx<0||sx>=TW||sy<0||sy>=TH) return;
    distance_map[sy][sx]=0; qx[tail]=sx; qy[tail++]=sy;
    static const int vx[]={1,-1,0,0},vy[]={0,0,1,-1};
    while(head<tail) {
        int x=qx[head],y=qy[head++];
        for(int i=0;i<4;i++) {int nx=x+vx[i],ny=y+vy[i];
            if(nx>=0&&nx<TW&&ny>=0&&ny<TH&&world[ny][nx]!='#'&&distance_map[ny][nx]<0) {
                distance_map[ny][nx]=distance_map[y][x]+1; qx[tail]=nx; qy[tail++]=ny;
            }
        }
    }
}
static void spawn_enemy() {
    int active=0; for(const auto& e:enemies) if(e.active) active++;
    if(active>=difficulties[difficulty].enemy_limit) return;
    static const int positions[][2]={{17,2},{2,13},{17,13},{10,1},{1,8},{18,7}};
    for(auto& e:enemies) if(!e.active) {
        for(int n=0;n<12;n++) { auto& p=positions[rnd()%6]; float x=float(p[0]*16+8), y=float(p[1]*16+8);
            if(!blocked(x,y,5) && length(px-x,py-y)>80) {
                int type=(level>=10 && rnd()%6==0)?5:(level>=6 && rnd()%5==0)?4:(level>=2 && rnd()%5==0)?3:((level>=1 && rnd()%3==0)?1:((level>=3 && rnd()%4==0)?2:0));
                e={x,y,2+std::min(5,level/4)+(type==2?3:0),0,0,true,type,60+int(rnd()%80)}; return;
            }
        } return;
    }
}
static void fire() {
    if(shots>0) return;
    int count=weapon?3:1, made=0;
    for(auto& b:bullets) if(!b.active && made<count) {float spread=weapon?float(made-1)*0.30f:0; b={px+fx*8,py+fy*8,(fx-fy*spread)*3.1f,(fy+fx*spread)*3.1f,weapon?42:70,true};made++;}
    if(made) {shots=weapon?25:11;burst(px+fx*8,py+fy*8,color(255,211,114),weapon?6:3);sound(0);}
}
static void enemy_fire(float x,float y,float dx,float dy,float speed) {
    float m=length(dx,dy); if(m<0.01f) return;
    for(auto& b:enemy_bullets) if(!b.active) { b={x,y,dx/m*speed,dy/m*speed,150,true}; return; }
}
static bool in_sword_arc(float x,float y) {
    float dx=x-px,dy=y-py,dist=length(dx,dy);
    return dist<29 && (dist<8 || (dx*fx+dy*fy)>dist*0.25f);
}

// ---- Scoring: chained kills raise a score multiplier (x1..x4); taking damage breaks the chain ----
static int combo_mult() { return 1+std::min(combo,9)/3; }
static void kill_enemy(Enemy& e, bool by_sword) {
    e.active=false;
    static const int points[]={25,30,60,45,40,35};
    score+=(points[e.type]+(by_sword?10:0))*combo_mult(); combo++; combo_timer=180;
    burst(e.x,e.y,color(255,154,100),10);
    // The sword cuts clean: only shots make a yellow enemy split into a ring.
    if(e.type==3 && !by_sword) for(int i=0;i<6;i++) {float a=float(i)*1.04719755f;enemy_fire(e.x,e.y,cosf(a),sinf(a),1.25f);}
    if(pickup_life==0 && health<MAX_HP && rnd()%7==0) {pickup_x=int(e.x);pickup_y=int(e.y);pickup_life=600;}
}
static void hit_boss(float x,float y,int damage) {
    boss_health-=damage; boss_hurt=5; burst(x,y,color(255,185,113),4);
    if(boss_health<=0 && !boss_defeated) {boss_defeated=true;score+=1000;burst(152,120,color(255,198,119),25);sound(3);}
}
static bool damage_player() { // returns true when the run ended
    health--; invulnerable=80; shake=8; combo=0; combo_timer=0; sound(2); burst(px,py,color(255,106,112),12);
    if(health<=0) { record_progress(); scene=DEAD; return true; }
    return false;
}
static void swing_sword() {
    if(sword_cooldown>0) return;
    sword_cooldown=32; sword_timer=10; sound(4);
    for(auto& b:enemy_bullets) if(b.active && in_sword_arc(b.x,b.y)) {b.active=false;burst(b.x,b.y,color(160,232,255),3);}
    for(auto& e:enemies) if(e.active && in_sword_arc(e.x,e.y)) {
        e.hp-=3+damage_upgrade; e.hurt=8; burst(e.x,e.y,color(255,200,130),8);
        if(e.hp<=0) kill_enemy(e,true);
    }
    if(is_boss_stage() && !boss_defeated && in_sword_arc(152,120)) hit_boss(152,120,3+damage_upgrade);
}
static void update_game() {
    ++elapsed;
    if (pressed[VK_ESCAPE] || pressed['P']) { pause_choice=0; scene=PAUSED; return; }
    if(pressed['E']) {weapon=1-weapon;sound(3);}
    if(sword_timer>0) sword_timer--;
    if(sword_cooldown>0) sword_cooldown--;
    if(boss_hurt>0) boss_hurt--;
    if(combo_timer>0 && --combo_timer==0) combo=0;
    if(pressed['F']) swing_sword();
    float dx=float(keys['D']||keys[VK_RIGHT])-float(keys['A']||keys[VK_LEFT]);
    float dy=float(keys['S']||keys[VK_DOWN])-float(keys['W']||keys[VK_UP]);
    float mag=length(dx,dy);
    if(mag>0) { dx/=mag; dy/=mag; if(dash_frames==0) {fx=dx; fy=dy;} }
    if(pressed[VK_SHIFT] && dash_cooldown==0) { dash_frames=10; dash_cooldown=std::max(45,120-dash_upgrade*25); invulnerable=std::max(invulnerable,14); burst(px,py,color(90,200,240),8); }
    if(dash_frames>0) { move_entity(px,py,fx*3.0f,fy*3.0f,5); dash_frames--; }
    else if(mag>0) { float speed=difficulties[difficulty].player_speed+speed_upgrade*0.16f; move_entity(px,py,dx*speed,dy*speed,5); }
    if(dash_cooldown>0) dash_cooldown--;
    if(keys[VK_SPACE]) fire();
    if(shots>0) --shots;
    if(invulnerable>0) --invulnerable;
    if(shake>0) --shake;
    if(elapsed%12==0) update_distance();
    if(--spawn_timer<=0) { spawn_enemy(); spawn_timer=int(std::max(58,165-level*14-collected*6)*difficulties[difficulty].spawn_factor); }
    for(auto& c:crystals) if(!c.collected && length(px-c.x,py-c.y)<9) { c.collected=true; collected++; score+=100; sound(1); burst(float(c.x),float(c.y),color(98,242,216),12); }
    if(level>=2 && invulnerable==0 && (elapsed%180)<75) {
        static const int hazards[][2]={{7*16+8,5*16+8},{13*16+8,9*16+8}};
        for(auto& h:hazards) if(length(px-h[0],py-h[1])<8) { if(damage_player()) return; break; }
    }
    if(pickup_life>0) {pickup_life--; if(length(px-pickup_x,py-pickup_y)<9) {health=std::min(MAX_HP,health+1);pickup_life=0;burst(px,py,color(124,255,158),10);sound(1);}}
    if(collected==goal && (!is_boss_stage() || boss_defeated) && length(px-(17*16+8),py-(13*16+8))<10) {
        score+=250; sound(3); record_progress(); scene=(level==LEVELS-1)?WIN:STAGE_CLEAR; return;
    }
    for(auto& b:bullets) if(b.active) {
        b.x+=b.vx; b.y+=b.vy;
        if(--b.life<=0 || wall(b.x,b.y)) { b.active=false; continue; }
        if(is_boss_stage() && !boss_defeated && length(b.x-152,b.y-120)<11) { b.active=false; hit_boss(b.x,b.y,1+damage_upgrade); continue; }
        for(auto& e:enemies) if(e.active && length(e.x-b.x,e.y-b.y)<7) {
            b.active=false; e.hp-=1+damage_upgrade; e.hurt=6; burst(e.x,e.y,color(255,125,95),5);
            if(e.hp<=0) kill_enemy(e,false);
            break;
        }
    }
    for(auto& b:enemy_bullets) if(b.active) {
        b.x+=b.vx; b.y+=b.vy;
        if(--b.life<=0 || wall(b.x,b.y)) {b.active=false;continue;}
        if(length(px-b.x,py-b.y)<6) { b.active=false; if(invulnerable==0 && damage_player()) return; }
    }
    if(is_boss_stage() && !boss_defeated && --boss_cooldown<=0) {
        bool enraged=boss_health<=boss_max_health/2; boss_cooldown=enraged?65:105;
        int pattern=boss_data[campaign[level].boss].pattern;
        if(pattern==1) {
            float ex=px-152,ey=py-120;
            for(int i=-2;i<=2;i++) {float a=i*0.19f;enemy_fire(152,120,ex*cosf(a)-ey*sinf(a),ex*sinf(a)+ey*cosf(a),enraged?2.0f:1.55f);}
            boss_cooldown=enraged?48:70;
        } else {
            int count=pattern==2?(enraged?16:10):(enraged?12:8);
            for(int i=0;i<count;i++) {float a=float(i)*6.2831853f/float(count)+float(elapsed%180)*(pattern==2?0.04f:0.01f);enemy_fire(152,120,cosf(a),sinf(a),enraged?1.65f:1.35f);}
            if(enraged || pattern==2) enemy_fire(152,120,px-152,py-120,2.1f);
            if(pattern==2) boss_cooldown=enraged?50:80;
        }
    }
    for(auto& e:enemies) if(e.active) {
        float ex=px-e.x,ey=py-e.y,m=length(ex,ey);
        if(e.type==1 && --e.cooldown<=0 && m<140) {enemy_fire(e.x,e.y,ex,ey,1.65f);e.cooldown=110+int(rnd()%45);}
        if(e.type==4 && --e.cooldown<=0 && m<150) {enemy_fire(e.x,e.y,ex,ey,1.5f);enemy_fire(e.x,e.y,ex-ey*0.3f,ey+ex*0.3f,1.5f);enemy_fire(e.x,e.y,ex+ey*0.3f,ey-ex*0.3f,1.5f);e.cooldown=140;}
        int tx=int(e.x)/16,ty=int(e.y)/16;
        float targetx=px,targety=py;
        if(m>20 && tx>0&&tx<TW-1&&ty>0&&ty<TH-1) {
            static const int vx[]={1,-1,0,0},vy[]={0,0,1,-1};
            int bx=tx,by=ty,best=distance_map[ty][tx];
            for(int i=0;i<4;i++) {int nx=tx+vx[i],ny=ty+vy[i],d=distance_map[ny][nx]; if(d>=0&&(best<0||d<best)){best=d;bx=nx;by=ny;}}
            // First center in the current tile to avoid clipping pillar corners.
            float cx=tx*16+8.0f,cy=ty*16+8.0f;
            if(length(e.x-cx,e.y-cy)>2.0f && e.delay==0) {targetx=cx;targety=cy;}
            else { e.delay=1; targetx=bx*16+8.0f; targety=by*16+8.0f; if(length(e.x-targetx,e.y-targety)<2.0f) e.delay=0; }
        }
        float edx=targetx-e.x,edy=targety-e.y,dist=length(edx,edy);
        if(dist>0.1f) { float speed=(0.28f+std::min(level,12)*0.035f+collected*0.012f)*(e.type==2?0.72f:e.type==3?1.65f:e.type==5?((elapsed/45)%2?1.9f:0.45f):1.0f); speed*=difficulties[difficulty].enemy_speed; move_entity(e.x,e.y,edx/dist*speed,edy/dist*speed,5); }
        if(e.hurt>0) e.hurt--;
        if(m<11 && invulnerable==0 && damage_player()) return;
    }
    for(auto& p:particles) if(p.life>0) {p.x+=p.vx;p.y+=p.vy;p.vx*=0.97f;p.vy*=0.97f;p.life--;}
}

// ---- Drawing ----
static const uint32_t UI_PANEL=0x0a121b, UI_FRAME=0x73a0a3, UI_HINT=0x9cb8bf, UI_ITEM=0xffdb92, UI_TITLE=0x85f1d5;
static void panel(int x,int y,int w,int h) { rect(x,y,w,h,UI_PANEL); box(x,y,w,h,UI_FRAME); }
static void draw_world() {
    static const uint32_t floors[][2]={{0x111923,0x131d28},{0x1a192b,0x211e31},{0x1c211d,0x242a22},{0x271d20,0x302326},{0x17242a,0x1d2c30},{0x271a28,0x30202e}};
    static const uint32_t walls[]={0x32424b,0x50485e,0x4c5946,0x674b49,0x3b5962,0x654454};
    int theme=campaign[level].theme;
    const uint32_t floor1=floors[theme][0],floor2=floors[theme][1],wall1=walls[theme],wall2=color(33,45,56);
    // Screen shake moves the playfield only; the HUD and menus stay still.
    if(shake>0) {cam_x=((frame_number*7)%3-1)*(shake>4?2:1);cam_y=((frame_number*5)%3-1)*(shake>4?2:1);}
    rect(TILE,TILE,W-2*TILE,H-2*TILE,floor1); // one quad for the whole floor; only the alternate tiles are added on top
    for(int y=0;y<TH;y++) for(int x=0;x<TW;x++) {
        int sx=x*16,sy=y*16;
        if(world[y][x]=='#') {
            rect(sx,sy,16,16,wall2); rect(sx,sy,16,12,wall1); rect(sx,sy+11,16,2,color(24,32,43));
            rect(sx+1,sy+1,14,1,color(74,91,95)); if((x+y)%2==0) rect(sx+8,sy+5,6,1,color(61,77,84));
        } else {
            if((x+y)%2==0) rect(sx,sy,16,16,floor2);
            if((x*7+y*13)%9==0) {pixel(sx+3,sy+5,color(37,50,54));pixel(sx+10,sy+12,color(38,49,54));}
        }
    }
    int ex=17*16+8,ey=13*16+8; uint32_t door=(collected==goal && (!is_boss_stage() || boss_defeated))?color(121,255,199):color(115,84,90);
    rect(ex-7,ey-7,14,14,color(9,16,23));box(ex-7,ey-7,14,14,door);rect(ex-3,ey-4,6,8,door);
    if(level>=2) { static const int hazards[][2]={{7*16+8,5*16+8},{13*16+8,9*16+8}}; bool active=elapsed%180<75;
        for(auto& h:hazards) {rect(h[0]-7,h[1]-7,14,14,color(56,38,44));box(h[0]-7,h[1]-7,14,14,active?color(255,116,85):color(100,67,69)); if(active){rect(h[0]-4,h[1]-1,8,3,color(255,183,103));rect(h[0]-1,h[1]-4,3,8,color(255,183,103));}} }
    for(auto& c:crystals) if(!c.collected) {
        int bob=(frame_number/16)%2; rect(c.x-5,c.y-5+bob,10,10,color(22,59,66));
        rect(c.x-2,c.y-5+bob,4,10,color(90,232,215)); rect(c.x-4,c.y-2+bob,8,4,color(144,255,232));
    }
    for(auto& b:bullets) if(b.active) { rect(int(b.x)-2,int(b.y)-2,4,4,color(255,221,148)); }
    for(auto& b:enemy_bullets) if(b.active) {rect(int(b.x)-2,int(b.y)-2,5,5,color(255,105,100));rect(int(b.x)-1,int(b.y)-1,2,2,color(255,215,155));}
    if(pickup_life>0 && (pickup_life>120 || (frame_number/8)%2)) {rect(pickup_x-5,pickup_y-5,11,11,color(35,94,63));rect(pickup_x-1,pickup_y-4,3,9,color(145,255,169));rect(pickup_x-4,pickup_y-1,9,3,color(145,255,169));}
    if(is_boss_stage() && !boss_defeated) {
        int pulse=(frame_number/12)%2;
        rect(140,109,24,23,color(80,37,61)); rect(142,107,20,20,boss_hurt>0?color(255,240,220):pulse?color(235,135,135):boss_data[campaign[level].boss].color);
        rect(145,111,14,12,color(77,33,56)); rect(148,115,8,4,color(255,193,135));
        rect(137,125,7,4,color(216,112,115)); rect(160,125,7,4,color(216,112,115));
    }
    for(auto& e:enemies) if(e.active) {
        int x=int(e.x),y=int(e.y); uint32_t c=e.hurt?color(255,232,199):(e.type==1?color(210,136,232):e.type==2?color(219,159,85):e.type==3?color(245,198,85):e.type==4?color(100,211,169):e.type==5?color(108,155,245):color(222,95,101));
        rect(x-6,y-5,12,10,color(76,37,47)); rect(x-5,y-6,10,10,c); rect(x-4,y-2,2,2,color(255,231,193)); rect(x+2,y-2,2,2,color(255,231,193));
        rect(x-4,y+4,3,3,color(144,59,69)); rect(x+1,y+4,3,3,color(144,59,69));
    }
    if(invulnerable==0 || (invulnerable/5)%2==0) {
        int x=int(px),y=int(py); rect(x-5,y-7,10,13,color(55,120,168));rect(x-4,y-6,8,9,player_colors[player_color]);
        rect(x-3,y-4,6,3,color(227,245,230)); rect(x-4,y+5,3,3,color(33,72,112));rect(x+1,y+5,3,3,color(33,72,112));
        rect(x+int(fx*7)-1,y+int(fy*7)-1,3,3,color(255,221,148));
    }
    if(sword_timer>0) {for(int i=0;i<5;i++) {float a=(float(i)-2)*0.30f;float dx=fx*cosf(a)-fy*sinf(a),dy=fx*sinf(a)+fy*cosf(a);rect(int(px+dx*21)-2,int(py+dy*21)-2,5,5,color(184,244,255));}}
    for(auto& p:particles) if(p.life>0) rect(int(p.x),int(p.y),2,2,p.c);
    cam_x=cam_y=0;
    if(is_boss_stage() && !boss_defeated) {
        rect(98,18,124,12,color(10,17,25)); text(101,20,boss_data[campaign[level].boss].name,color(255,182,154));
        rect(145,21,72,5,color(78,39,48)); rect(145,21,std::max(0,boss_health)*72/boss_max_health,5,color(247,112,109));
    }
    rect(0,0,W,15,color(10,17,25)); rect(0,14,W,1,color(67,80,86));
    text(5,4,"HP",color(175,196,200)); for(int i=0;i<MAX_HP;i++) rect(23+i*7,4,5,7,i<health?color(245,108,110):color(67,48,56));
    char hud[64]; snprintf(hud,sizeof(hud),"CORE %d/%d",collected,goal); text(70,4,hud,color(122,234,214));
    snprintf(hud,sizeof(hud),"LV %d/%d",level+1,LEVELS); text(135,4,hud,color(243,206,153));
    snprintf(hud,sizeof(hud),"SCORE %d",score); text(215,4,hud,color(224,219,180));
    rect(5,18,76,10,color(10,17,25)); text(7,20,dash_cooldown?"DASH WAIT":"DASH READY",dash_cooldown?color(143,145,150):color(125,227,239));
    rect(230,18,85,10,color(10,17,25)); text(232,20,weapon?"SPREAD":"PULSE",color(243,206,153));
    if(combo_mult()>1 && combo_timer>0) {
        snprintf(hud,sizeof(hud),"CHAIN X%d",combo_mult()); rect(5,30,58,12,color(10,17,25)); text(8,32,hud,color(255,196,120));
        rect(5,41,std::max(1,combo_timer*58/180),1,color(255,150,90));
    }
}
static void draw() {
    vertex_count=0; draw_world();
    char line[64];
    if(scene==TITLE) {
        panel(23,32,274,175);
        centered(48,"ECHO VAULT",color(130,235,222),2);
        centered(74,"36 SECTORS SIX GUARDIANS",color(202,211,198));
        rect(50,90,220,1,color(64,89,95));
        centered(98,"COLLECT CORES AND REACH EXIT",color(243,220,160));
#ifdef EV_TOUCH
        centered(113,"TOUCH PAD TO MOVE",UI_HINT);
        centered(124,"FIRE  SWORD  DASH",UI_HINT);
        centered(135,"GUN SWITCHES YOUR WEAPON",UI_HINT);
        centered(146,"PAUSE AND START ON SCREEN",UI_HINT);
        centered(160,"SET SETTINGS  SOUND ON SCREEN",color(255,167,140));
#else
        centered(113,"WASD OR ARROWS  MOVE",UI_HINT);
        centered(124,"SPACE FIRE   E CHANGE WEAPON",UI_HINT);
        centered(135,"F SWORD   SHIFT DASH   M SOUND",UI_HINT);
        centered(146,"P OR ESC PAUSE   F11 FULLSCREEN",UI_HINT);
        centered(160,"C SETTINGS   GAMEPAD SUPPORTED",color(255,167,140));
#endif
        snprintf(line,sizeof(line),"BEST %d  STAGE %d/%d",best_score[difficulty],best_stage[difficulty],LEVELS);
        centered(174,line,color(224,219,180));
        if((frame_number/30)%2==0) centered(192,
#ifdef EV_TOUCH
            "TAP START TO BEGIN"
#else
            "PRESS ENTER TO START"
#endif
            ,color(255,226,153));
    } else if(scene==PAUSED) {
        panel(45,57,230,126);
        centered(68,"PAUSED",UI_TITLE,2);
        centered(100,pause_choice==0?"> RESUME":"  RESUME",UI_ITEM);
        centered(120,pause_choice==1?"> GAME GUIDE":"  GAME GUIDE",UI_ITEM);
        centered(140,pause_choice==2?"> SETTINGS":"  SETTINGS",UI_ITEM);
        centered(162,"UP DOWN ENTER",UI_HINT);
    } else if(scene==SETTINGS) {
        panel(25,39,270,162);
        centered(52,"SETTINGS",UI_TITLE,2);
        snprintf(line,sizeof(line),"%s DIFFICULTY: %s",settings_choice==0?">":" ",difficulties[difficulty].name);text(43,82,line,UI_ITEM);
        snprintf(line,sizeof(line),"%s PLAYER COLOR: %s",settings_choice==1?">":" ",color_names[player_color]);text(43,99,line,UI_ITEM);
        snprintf(line,sizeof(line),"%s START GUN: %s",settings_choice==2?">":" ",starting_weapon?"SPREAD":"PULSE");text(43,116,line,UI_ITEM);
        snprintf(line,sizeof(line),"%s SOUND: %s",settings_choice==3?">":" ",muted?"OFF":"ON");text(43,133,line,UI_ITEM);
        text(43,150,settings_choice==4?"> BACK":"  BACK",UI_ITEM);
        centered(177,"UP DOWN SELECT  LEFT RIGHT CHANGE",UI_HINT);
    } else if(scene==HELP) {
        panel(10,8,300,224);
        snprintf(line,sizeof(line),"GAME GUIDE %d/3",help_page+1);
        centered(19,line,UI_TITLE,2);
        static const char* pages[3][9]={
            {"MOVE: WASD / ARROWS / JOYSTICK","SPACE / FIRE: SHOOT IN FACING DIRECTION","E / GUN: SWITCH PULSE AND SPREAD","F / SWORD: HIT AND BLOCK BULLETS","SHIFT / DASH: QUICK MOVE AND DODGE","P ESC / PAUSE: PAUSE OR RESUME","M / SOUND: TOGGLE SOUND","ENTER / START: CONFIRM OR RETRY",nullptr},
            {"COLLECT ALL CORES TO OPEN THE EXIT","EVERY SIXTH STAGE HAS A GUARDIAN","GREEN CROSS PICKUPS RESTORE HEALTH","RED FLOOR TRAPS TURN ON AND OFF","PURPLE SHOOTS  ORANGE IS ARMORED","YELLOW IS FAST  GREEN FIRES 3 SHOTS","BLUE CHARGES  BOSSES ENRAGE AT HALF HP","AFTER EACH STAGE CHOOSE AN UPGRADE",nullptr},
            {"FIREPOWER: INCREASE ATTACK DAMAGE","MOVE SPEED: WALK FASTER","FASTER DASH: SHORTER DASH RECHARGE","REPAIR: RESTORE TWO HP","PULSE: FAST SINGLE SHOTS","SPREAD: THREE SHORT RANGE SHOTS","KILL CHAINS RAISE SCORE MULTIPLIER","CLEAR A STAGE TO REGAIN ONE HP","ON DEFEAT START RETRIES THE STAGE"}};
        for(int i=0;i<9;i++) if(pages[help_page][i]) text(20,45+i*16,pages[help_page][i],color(206,218,208));
        centered(185,"LEFT RIGHT: CHANGE PAGE",UI_HINT);
        box(75,200,170,20,UI_FRAME); centered(206,"ENTER / H: BACK TO PAUSE",UI_ITEM);
    } else if(scene==STAGE_CLEAR) {
        panel(25,50,270,142);
        centered(60,"SECTOR CLEAR",UI_TITLE,2);
        centered(82,"CHOOSE AN UPGRADE",UI_ITEM);
        static const char* names[]={"FIREPOWER","MOVE SPEED","FASTER DASH","REPAIR +2 HP"};
        for(int i=0;i<4;i++) {snprintf(line,sizeof(line),"%s %s",upgrade==i?">":" ",names[i]);text(48,102+i*16,line,upgrade==i?color(255,226,153):UI_HINT);}
        centered(172,
#ifdef EV_TOUCH
            "TOUCH UP DOWN AND START"
#else
            "UP DOWN AND ENTER"
#endif
            ,color(206,218,208));
    } else if(scene==DEAD || scene==WIN) {
        panel(45,78,230,92);
        centered(88,scene==DEAD?"SIGNAL LOST":"ESCAPE COMPLETE",scene==DEAD?color(255,130,129):UI_TITLE,2);
        snprintf(line,sizeof(line),"SCORE %d  TIME %d SEC",score,elapsed/60); centered(110,line,color(208,215,201));
        if(new_best) centered(124,"NEW BEST!",color(124,255,158));
        else {snprintf(line,sizeof(line),"BEST %d",best_score[difficulty]);centered(124,line,UI_HINT);}
        centered(145,
#ifdef EV_TOUCH
            scene==DEAD?"START TO RETRY SECTOR":"START TO PLAY AGAIN"
#else
            scene==DEAD?"ENTER TO RETRY SECTOR":"ENTER TO PLAY AGAIN"
#endif
            ,UI_ITEM);
    }
}

// ---- Scene logic and input (shared by every platform) ----
static void apply_upgrade_and_advance() {
    if(upgrade==0) damage_upgrade++; else if(upgrade==1) speed_upgrade++; else if(upgrade==2) dash_upgrade++; else health=std::min(MAX_HP,health+2);
    level++; health=std::min(MAX_HP,health+1); begin_level(); scene=PLAY;
}
// Pointer click in logical 320x240 coordinates. Handles menus only (title/dead/win are the platform's job).
// Returns true when the click hit a menu control, so the platform can ignore it for other purposes.
static bool ui_click(int x,int y) {
    if(scene==HELP && x>=75 && x<245 && y>=200 && y<220) {scene=PAUSED;return true;}
    if(scene==PAUSED && x>=45 && x<275 && y>=95 && y<155) {
        int row=(y-95)/20; pause_choice=row;
        if(row==0) scene=PLAY; else if(row==1) {help_page=0;scene=HELP;} else open_settings(PAUSED);
        return true;
    }
    if(scene==SETTINGS && x>=40 && x<280 && y>=79 && y<164) {
        settings_choice=(y-79)/17;
        if(settings_choice==4) scene=settings_return; else change_setting(1);
        return true;
    }
    if(scene==STAGE_CLEAR && x>=40 && x<280 && y>=100 && y<166) {
        int row=std::min(3,(y-100)/16);
        if(row==upgrade) apply_upgrade_and_advance(); else upgrade=row;
        return true;
    }
    return false;
}
// Called when the app loses focus or goes to the background.
static void auto_pause() { if(scene==PLAY) {pause_choice=0;scene=PAUSED;} memset(keys,0,sizeof(keys)); }
// One fixed 1/60 s step. The caller clears `pressed` afterwards.
static void step() {
    ++frame_number;
    if(pressed['M']) toggle_mute();
    switch(scene) {
    case TITLE:
        if(pressed[VK_RETURN]) {reset_game();scene=PLAY;}
        else if(pressed['C']) open_settings(TITLE);
        break;
    case DEAD: if(pressed[VK_RETURN]) {retry_level();scene=PLAY;} break;
    case WIN: if(pressed[VK_RETURN]) {reset_game();scene=PLAY;} break;
    case STAGE_CLEAR:
        if(pressed[VK_UP]||pressed['W']) upgrade=(upgrade+3)%4;
        if(pressed[VK_DOWN]||pressed['S']) upgrade=(upgrade+1)%4;
        if(pressed[VK_RETURN]) apply_upgrade_and_advance();
        break;
    case PAUSED:
        if(pressed[VK_UP]||pressed['W']) pause_choice=(pause_choice+2)%3;
        if(pressed[VK_DOWN]||pressed['S']) pause_choice=(pause_choice+1)%3;
        if(pressed['H']) {help_page=0;scene=HELP;}
        else if(pressed['C']) open_settings(PAUSED);
        else if(pressed[VK_RETURN]) {if(pause_choice==0) scene=PLAY;else if(pause_choice==1) {help_page=0;scene=HELP;}else open_settings(PAUSED);}
        else if(pressed['P']||pressed[VK_ESCAPE]) scene=PLAY;
        break;
    case SETTINGS: update_settings(); break;
    case HELP:
        if(pressed[VK_LEFT]||pressed['A']) help_page=(help_page+2)%3;
        else if(pressed[VK_RIGHT]||pressed['D']) help_page=(help_page+1)%3;
        if(pressed[VK_RETURN]||pressed['H']||pressed['P']||pressed[VK_ESCAPE]) scene=PAUSED;
        break;
    case PLAY: update_game(); break;
    }
}
