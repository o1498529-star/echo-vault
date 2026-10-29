#pragma once

// Campaign data is kept outside the gameplay code so stages and bosses can be edited separately.
struct StageData { int seed, theme, boss; };
struct BossData { const char* name; int pattern; unsigned color; };
static const BossData boss_data[] = {
    {"WARDEN",0,0xb04468}, {"HUNTER",1,0xc66542}, {"ORACLE",2,0x8163bb},
    {"TITAN",0,0xa47b3d}, {"HYDRA",1,0x398f81}, {"NEMESIS",2,0xc34a53}
};
#define STAGE(seed,theme,boss) {seed,theme,boss}
static const StageData campaign[] = {
 STAGE(0,0,-1),STAGE(1,0,-1),STAGE(2,0,-1),STAGE(3,0,-1),STAGE(4,0,-1),STAGE(5,0,0),
 STAGE(6,1,-1),STAGE(7,1,-1),STAGE(8,1,-1),STAGE(9,1,-1),STAGE(10,1,-1),STAGE(11,1,1),
 STAGE(12,2,-1),STAGE(13,2,-1),STAGE(14,2,-1),STAGE(15,2,-1),STAGE(16,2,-1),STAGE(17,2,2),
 STAGE(18,3,-1),STAGE(19,3,-1),STAGE(20,3,-1),STAGE(21,3,-1),STAGE(22,3,-1),STAGE(23,3,3),
 STAGE(24,4,-1),STAGE(25,4,-1),STAGE(26,4,-1),STAGE(27,4,-1),STAGE(28,4,-1),STAGE(29,4,4),
 STAGE(30,5,-1),STAGE(31,5,-1),STAGE(32,5,-1),STAGE(33,5,-1),STAGE(34,5,-1),STAGE(35,5,5)
};
#undef STAGE
constexpr int LEVELS=sizeof(campaign)/sizeof(campaign[0]);
