#include <jni.h>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
constexpr int VK_SHIFT=16, VK_ESCAPE=27, VK_SPACE=32, VK_LEFT=37, VK_UP=38, VK_RIGHT=39, VK_DOWN=40, VK_RETURN=13;
template <size_t N, typename... A> int sprintf_s(char (&b)[N], const char* fmt, A... args) { return snprintf(b,N,fmt,args...); }
// Native Win32 + software pixels. No external assets or libraries.
constexpr int W = 320, H = 240, TW = 20, TH = 15, TILE = 16;
constexpr float STEP = 1.0f / 60.0f;
static uint32_t framebuffer[W * H];
static bool keys[256], pressed[256], running = true;
static uint32_t random_state = 0x17ac091u;
static int frame_number = 0, shake = 0;
static int pending_sound=0; static bool muted=false;
static void sound(int id) { if(!muted) pending_sound=id+1; }
static unsigned rnd() { random_state ^= random_state << 13; random_state ^= random_state >> 17; random_state ^= random_state << 5; return random_state; }
static uint32_t color(int r, int g, int b) { return (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b); }
static void pixel(int x, int y, uint32_t c) { if ((unsigned)x < W && (unsigned)y < H) framebuffer[y * W + x] = c; }
static void rect(int x, int y, int w, int h, uint32_t c) {
    int x0 = std::max(0, x), y0 = std::max(0, y), x1 = std::min(W, x + w), y1 = std::min(H, y + h);
    for (int j = y0; j < y1; ++j) for (int i = x0; i < x1; ++i) framebuffer[j * W + i] = c;
}
static void box(int x, int y, int w, int h, uint32_t c) { rect(x,y,w,1,c); rect(x,y+h-1,w,1,c); rect(x,y,1,h,c); rect(x+w-1,y,1,h,c); }

// 5x7 bitmap alphabet, stored in ROM-like static data.
static const char* glyph(char c) {
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
    case '/':return "00001/00010/00010/00100/01000/01000/10000";
    case '-':return "00000/00000/00000/11111/00000/00000/00000";
    case '>':return "10000/01000/00100/00010/00100/01000/10000";
    default:return "00000/00000/00000/00000/00000/00000/00000";
    }
}
static void text(int x, int y, const char* s, uint32_t c, int scale=1) {
    for (; *s; ++s, x += 6 * scale) {
        const char* g = glyph(*s);
        for (int row = 0; row < 7; ++row) for (int col = 0; col < 5; ++col)
            if (g[row * 6 + col] == '1') rect(x + col*scale, y + row*scale, scale, scale, c);
    }
}
static void centered(int y, const char* s, uint32_t c, int scale=1) { text((W - int(strlen(s))*6*scale)/2, y, s, c, scale); }

constexpr int LEVELS=24;
static char world[TH][TW+1];
static int distance_map[TH][TW];
struct Bullet { float x,y,vx,vy; int life; bool active; };
struct Enemy { float x,y; int hp, hurt, delay; bool active; int type, cooldown; };
struct Particle { float x,y,vx,vy; int life; uint32_t c; };
struct Crystal { int x,y; bool collected; };
static Bullet bullets[24], enemy_bullets[32]; static Enemy enemies[12]; static Particle particles[80];
static Crystal crystals[5];
static float px,py,fx,fy; static int health, collected, shots, invulnerable, spawn_timer, score, elapsed;
static int dash_cooldown=0, dash_frames=0, boss_health=0, boss_cooldown=0, pickup_x=0, pickup_y=0, pickup_life=0;
static bool boss_defeated=false;
static int weapon=0, upgrade=0, damage_upgrade=0, speed_upgrade=0, dash_upgrade=0, sword_timer=0, sword_cooldown=0, boss_max_health=24;
static int level=0, goal=3, level_start_score=0, level_start_elapsed=0, level_start_health=5;
static bool is_boss_stage() {return (level+1)%6==0;}
enum Scene { TITLE, PLAY, DEAD, STAGE_CLEAR, WIN, PAUSED }; static Scene scene = TITLE;
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
    make_world(level);
    // Odd tile coordinates are always connected by the corridor grid.
    goal=std::min(5,3+level/4);
    for(int i=0;i<5;i++) {int cell=(level*11+i*13+8)%62+1;int tx=(cell%9)*2+1,ty=(cell/9)*2+1;crystals[i]={tx*16+8,ty*16+8,i>=goal};}
    px=24; py=24; fx=1; fy=0; collected=0; shots=0; invulnerable=0; spawn_timer=100; shake=0;
    dash_cooldown=0; dash_frames=0; sword_timer=0; sword_cooldown=0; boss_max_health=24+(level/6)*8; boss_health=boss_max_health; boss_cooldown=100; boss_defeated=false; pickup_life=0;
    level_start_score=score; level_start_elapsed=elapsed; level_start_health=health;
}
static void reset_game() {
    level=0; health=5; score=0; elapsed=0; weapon=0; upgrade=0; damage_upgrade=0; speed_upgrade=0; dash_upgrade=0; begin_level();
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
static void swing_sword() {
    if(sword_cooldown>0) return;
    sword_cooldown=32; sword_timer=10; sound(4);
    for(auto& b:enemy_bullets) if(b.active && in_sword_arc(b.x,b.y)) {b.active=false;burst(b.x,b.y,color(160,232,255),3);}
    for(auto& e:enemies) if(e.active && in_sword_arc(e.x,e.y)) {
        e.hp-=3+damage_upgrade; e.hurt=8; burst(e.x,e.y,color(255,200,130),8);
        if(e.hp<=0) {e.active=false;score+=e.type==2?60:35;burst(e.x,e.y,color(255,154,100),10);}
    }
    if(is_boss_stage() && !boss_defeated && in_sword_arc(152,120)) {
        boss_health-=3+damage_upgrade;burst(152,120,color(255,185,113),8);
        if(boss_health<=0) {boss_defeated=true;score+=1000;burst(152,120,color(255,198,119),25);}
    }
}
static void update_game() {
    ++elapsed;
    if (pressed[VK_ESCAPE] || pressed['P']) { scene=PAUSED; return; }
    if(pressed['E']) {weapon=1-weapon;sound(3);}
    if(sword_timer>0) sword_timer--; if(sword_cooldown>0) sword_cooldown--;
    if(pressed['F']) swing_sword();
    float dx=float(keys['D']||keys[VK_RIGHT])-float(keys['A']||keys[VK_LEFT]);
    float dy=float(keys['S']||keys[VK_DOWN])-float(keys['W']||keys[VK_UP]);
    float mag=length(dx,dy);
    if(mag>0) { dx/=mag; dy/=mag; if(dash_frames==0) {fx=dx; fy=dy;} }
    if(pressed[VK_SHIFT] && dash_cooldown==0) { dash_frames=10; dash_cooldown=std::max(45,120-dash_upgrade*25); invulnerable=std::max(invulnerable,14); burst(px,py,color(90,200,240),8); }
    if(dash_frames>0) { move_entity(px,py,fx*3.0f,fy*3.0f,5); dash_frames--; }
    else if(mag>0) move_entity(px,py,dx*(1.15f+speed_upgrade*0.16f),dy*(1.15f+speed_upgrade*0.16f),5);
    if(dash_cooldown>0) dash_cooldown--;
    if(keys[VK_SPACE]) fire();
    if(shots>0) --shots; if(invulnerable>0) --invulnerable; if(shake>0) --shake;
    if(elapsed%12==0) update_distance();
    if(--spawn_timer<=0) { spawn_enemy(); spawn_timer=std::max(58,165-level*14-collected*6); }
    for(auto& c:crystals) if(!c.collected && length(px-c.x,py-c.y)<9) { c.collected=true; collected++; score+=100; sound(1); burst(float(c.x),float(c.y),color(98,242,216),12); }
    if(level>=2 && invulnerable==0 && (elapsed%180)<75) {
        static const int hazards[][2]={{7*16+8,5*16+8},{13*16+8,9*16+8}};
        for(auto& h:hazards) if(length(px-h[0],py-h[1])<8) {health--;invulnerable=80;shake=8;sound(2);burst(px,py,color(255,106,112),12);if(health<=0){scene=DEAD;return;}break;}
    }
    if(pickup_life>0) {pickup_life--; if(length(px-pickup_x,py-pickup_y)<9) {health=std::min(5,health+1);pickup_life=0;burst(px,py,color(124,255,158),10);}}
    if(collected==goal && (!is_boss_stage() || boss_defeated) && length(px-(17*16+8),py-(13*16+8))<10) {
        score+=250; sound(3); scene=(level==LEVELS-1)?WIN:STAGE_CLEAR; return;
    }
    for(auto& b:bullets) if(b.active) {
        b.x+=b.vx; b.y+=b.vy;
        if(--b.life<=0 || wall(b.x,b.y)) { b.active=false; continue; }
        if(is_boss_stage() && !boss_defeated && length(b.x-152,b.y-120)<11) {
            b.active=false; boss_health-=1+damage_upgrade; burst(b.x,b.y,color(255,185,113),4);
            if(boss_health<=0) { boss_defeated=true; score+=1000; burst(152,120,color(255,198,119),25); }
            continue;
        }
        for(auto& e:enemies) if(e.active && length(e.x-b.x,e.y-b.y)<7) {
            b.active=false; e.hp-=1+damage_upgrade; e.hurt=6; burst(e.x,e.y,color(255,125,95),5);
            if(e.hp<=0) { e.active=false; score+=e.type==2?60:e.type==3?45:25; burst(e.x,e.y,color(255,154,100),10);
                if(e.type==3) for(int i=0;i<6;i++) {float a=float(i)*1.04719755f;enemy_fire(e.x,e.y,cosf(a),sinf(a),1.25f);}
                if(pickup_life==0 && health<5 && rnd()%7==0) {pickup_x=int(e.x);pickup_y=int(e.y);pickup_life=600;} } break;
        }
    }
    for(auto& b:enemy_bullets) if(b.active) {
        b.x+=b.vx; b.y+=b.vy;
        if(--b.life<=0 || wall(b.x,b.y)) {b.active=false;continue;}
        if(length(px-b.x,py-b.y)<6) { b.active=false; if(invulnerable==0) {health--;invulnerable=80;shake=8;sound(2);burst(px,py,color(255,106,112),12);if(health<=0){scene=DEAD;return;}} }
    }
    if(is_boss_stage() && !boss_defeated && --boss_cooldown<=0) {
        bool enraged=boss_health<=boss_max_health/2; boss_cooldown=enraged?65:105;
        int count=enraged?12:8;
        for(int i=0;i<count;i++) {float a=float(i)*6.2831853f/float(count)+float(elapsed%120)*0.01f;enemy_fire(152,120,cosf(a),sinf(a),enraged?1.65f:1.35f);}
        if(enraged) enemy_fire(152,120,px-152,py-120,2.1f);
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
        if(dist>0.1f) { float speed=(0.28f+std::min(level,12)*0.035f+collected*0.012f)*(e.type==2?0.72f:e.type==3?1.65f:e.type==5?((elapsed/45)%2?1.9f:0.45f):1.0f); move_entity(e.x,e.y,edx/dist*speed,edy/dist*speed,5); }
        if(e.hurt>0) e.hurt--;
        if(m<11 && invulnerable==0) { health--; invulnerable=80; shake=8; sound(2); burst(px,py,color(255,106,112),12); if(health<=0) {scene=DEAD;return;} }
    }
    for(auto& p:particles) if(p.life>0) {p.x+=p.vx;p.y+=p.vy;p.vx*=0.97f;p.vy*=0.97f;p.life--;}
}
static void draw_world() {
    static const uint32_t floors[][2]={{0x111923,0x131d28},{0x1a192b,0x211e31},{0x1c211d,0x242a22},{0x271d20,0x302326},{0x17242a,0x1d2c30},{0x271a28,0x30202e}};
    static const uint32_t walls[]={0x32424b,0x50485e,0x4c5946,0x674b49,0x3b5962,0x654454};
    int theme=(level/4)%6;
    const uint32_t floor1=floors[theme][0],floor2=floors[theme][1],wall1=walls[theme],wall2=color(33,45,56);
    for(int y=0;y<TH;y++) for(int x=0;x<TW;x++) {
        int sx=x*16,sy=y*16;
        if(world[y][x]=='#') {
            rect(sx,sy,16,16,wall2); rect(sx,sy,16,12,wall1); rect(sx,sy+11,16,2,color(24,32,43));
            rect(sx+1,sy+1,14,1,color(74,91,95)); if((x+y)%2==0) rect(sx+8,sy+5,6,1,color(61,77,84));
        } else {
            rect(sx,sy,16,16,(x+y)%2?floor1:floor2);
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
        rect(140,109,24,23,color(80,37,61)); rect(142,107,20,20,color(170+pulse*25,68,104));
        rect(145,111,14,12,color(77,33,56)); rect(148,115,8,4,color(255,193,135));
        rect(137,125,7,4,color(216,112,115)); rect(160,125,7,4,color(216,112,115));
        rect(98,18,124,12,color(10,17,25)); text(101,20,"WARDEN",color(255,182,154));
        rect(145,21,72,5,color(78,39,48)); rect(145,21,std::max(0,boss_health)*72/boss_max_health,5,color(247,112,109));
    }
    for(auto& e:enemies) if(e.active) {
        int x=int(e.x),y=int(e.y); uint32_t c=e.hurt?color(255,232,199):(e.type==1?color(210,136,232):e.type==2?color(219,159,85):e.type==3?color(245,198,85):e.type==4?color(100,211,169):e.type==5?color(108,155,245):color(222,95,101));
        rect(x-6,y-5,12,10,color(76,37,47)); rect(x-5,y-6,10,10,c); rect(x-4,y-2,2,2,color(255,231,193)); rect(x+2,y-2,2,2,color(255,231,193));
        rect(x-4,y+4,3,3,color(144,59,69)); rect(x+1,y+4,3,3,color(144,59,69));
    }
    if(invulnerable==0 || (invulnerable/5)%2==0) {
        int x=int(px),y=int(py); rect(x-5,y-7,10,13,color(55,120,168));rect(x-4,y-6,8,9,color(120,205,231));
        rect(x-3,y-4,6,3,color(227,245,230)); rect(x-4,y+5,3,3,color(33,72,112));rect(x+1,y+5,3,3,color(33,72,112));
        rect(x+int(fx*7)-1,y+int(fy*7)-1,3,3,color(255,221,148));
    }
    if(sword_timer>0) {for(int i=0;i<5;i++) {float a=(float(i)-2)*0.30f;float dx=fx*cosf(a)-fy*sinf(a),dy=fx*sinf(a)+fy*cosf(a);rect(int(px+dx*21)-2,int(py+dy*21)-2,5,5,color(184,244,255));}}
    for(auto& p:particles) if(p.life>0) rect(int(p.x),int(p.y),2,2,p.c);
    rect(0,0,W,15,color(10,17,25)); rect(0,14,W,1,color(67,80,86));
    text(5,4,"HP",color(175,196,200)); for(int i=0;i<5;i++) rect(23+i*7,4,5,7,i<health?color(245,108,110):color(67,48,56));
    char hud[64]; sprintf_s(hud,"CORE %d/%d",collected,goal); text(70,4,hud,color(122,234,214));
    sprintf_s(hud,"LV %d/%d",level+1,LEVELS); text(135,4,hud,color(243,206,153));
    sprintf_s(hud,"SCORE %d",score); text(215,4,hud,color(224,219,180));
    rect(5,18,76,10,color(10,17,25)); text(7,20,dash_cooldown?"DASH WAIT":"DASH READY",dash_cooldown?color(143,145,150):color(125,227,239));
    rect(230,18,85,10,color(10,17,25)); text(232,20,weapon?"SPREAD":"PULSE",color(243,206,153));
}
static void draw() {
    draw_world();
    if(scene==TITLE) {
        rect(23,32,274,175,color(11,20,30)); box(23,32,274,175,color(95,155,166));
        centered(48,"ECHO VAULT",color(130,235,222),2);
        centered(74,"24 SECTORS FOUR GUARDIANS",color(202,211,198));
        rect(50,90,220,1,color(64,89,95));
        centered(100,"COLLECT CORES AND REACH EXIT",color(243,220,160));
        centered(116,"TOUCH PAD TO MOVE",color(156,184,191));
        centered(128,"FIRE  SWORD  DASH",color(156,184,191));
        centered(140,"WEAPON SWITCHES YOUR GUN",color(156,184,191));
        centered(152,"PAUSE AND START ON SCREEN",color(156,184,191));
        centered(170,"BEWARE THE FLOOR TRAPS",color(255,167,140));
        if((frame_number/30)%2==0) centered(192,"TAP START TO BEGIN",color(255,226,153));
    } else if(scene==STAGE_CLEAR) {
        rect(25,50,270,142,color(10,18,27)); box(25,50,270,142,color(115,160,163));
        centered(62,"SECTOR CLEAR",color(133,241,213),2);
        centered(87,"CHOOSE AN UPGRADE",color(255,219,146));
        text(48,105,upgrade==0?"> FIREPOWER":"  FIREPOWER",upgrade==0?color(255,226,153):color(156,184,191));
        text(48,121,upgrade==1?"> MOVE SPEED":"  MOVE SPEED",upgrade==1?color(255,226,153):color(156,184,191));
        text(48,137,upgrade==2?"> FASTER DASH":"  FASTER DASH",upgrade==2?color(255,226,153):color(156,184,191));
        centered(165,"TOUCH UP DOWN AND START",color(206,218,208));
    } else if(scene==DEAD || scene==WIN || scene==PAUSED) {
        rect(45,78,230,92,color(10,18,27)); box(45,78,230,92,color(115,160,163));
        centered(91,scene==DEAD?"SIGNAL LOST":scene==WIN?"ESCAPE COMPLETE":scene==STAGE_CLEAR?"SECTOR CLEAR":"PAUSED",scene==DEAD?color(255,130,129):color(133,241,213),2);
        char info[40]; sprintf_s(info,"SCORE %d  TIME %d SEC",score,elapsed/60); centered(119,info,color(208,215,201));
        centered(141,scene==PAUSED?"PAUSE TO RESUME":scene==DEAD?"START TO RETRY SECTOR":scene==STAGE_CLEAR?"START FOR NEXT SECTOR":"START TO PLAY AGAIN",color(255,219,146));
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_echovault_game_GameView_nativeInit(JNIEnv*, jclass) { reset_game(); scene=TITLE; }
extern "C" JNIEXPORT void JNICALL Java_com_echovault_game_GameView_nativeSuspend(JNIEnv*, jclass) {if(scene==PLAY) scene=PAUSED; memset(keys,0,sizeof(keys));}
extern "C" JNIEXPORT jint JNICALL Java_com_echovault_game_GameView_nativeTick(JNIEnv*, jclass, jint held, jint pulse) {
    static const int codes[]={VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,VK_SPACE,'F',VK_SHIFT,'E','P',VK_RETURN,'M'};
    for(int i=0;i<11;i++) {bool down=(held & (1<<i))!=0; pressed[codes[i]]=(down&&!keys[codes[i]]) || (pulse & (1<<i));keys[codes[i]]=down;}
    ++frame_number;
    if(pressed['M']) muted=!muted;
    if(scene==TITLE && pressed[VK_RETURN]) {reset_game();scene=PLAY;}
    else if(scene==DEAD && pressed[VK_RETURN]) {retry_level();scene=PLAY;}
    else if(scene==WIN && pressed[VK_RETURN]) {reset_game();scene=PLAY;}
    else if(scene==STAGE_CLEAR) {
        if(pressed[VK_UP]) upgrade=(upgrade+2)%3;
        if(pressed[VK_DOWN]) upgrade=(upgrade+1)%3;
        if(pressed[VK_RETURN]) {if(upgrade==0) damage_upgrade++;else if(upgrade==1) speed_upgrade++;else dash_upgrade++;level++;health=std::min(5,health+1);begin_level();scene=PLAY;}
    } else if(scene==PAUSED && pressed['P']) scene=PLAY;
    else if(scene==PLAY) update_game();
    memset(pressed,0,sizeof(pressed)); int s=pending_sound;pending_sound=0;return s;
}
extern "C" JNIEXPORT void JNICALL Java_com_echovault_game_GameView_nativeDraw(JNIEnv* env, jclass, jintArray pixels) {
    draw(); for(auto& p:framebuffer) p|=0xff000000u;
    env->SetIntArrayRegion(pixels,0,W*H,reinterpret_cast<const jint*>(framebuffer));
}
