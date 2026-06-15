#include "raylib.h"
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#define SCREEN_W 1280
#define SCREEN_H 720
#define FLOOR_Y_RATIO 0.82f
#define PLAYER_GAP_X 550.0f
#define BOY_X  180.0f
#define GIRL_X (BOY_X + PLAYER_GAP_X)
#define P_GIRL_MAX_W 150.0f
#define P_GIRL_MAX_H 140.0f
#define P_BOY_MAX_W  150.0f
#define P_BOY_MAX_H  140.0f
#define BOY_SCALE_BONUS 1.08f
#define TITLE_BOY_SCALE_BONUS 1.06f
#define GRAVITY     2000.0f
#define JUMP_VY_GIRL (-780.0f)
#define JUMP_VY_BOY  (-830.0f)
#define JUMP2_FACTOR (0.90f)
#define MAX_JUMP_COUNT 2
#define SPEED_BASE  320.0f
#define SPEED_UP      8.0f
#define JAR_MAX_W    80.0f
#define JAR_MAX_H   105.0f
#define TANSU_MAX_W  420.0f
#define TANSU_MAX_H  200.0f
#define JICHO_MAX_W  260.0f
#define JICHO_MAX_H  140.0f
#define SMOKE_MAX_W  120.0f
#define SMOKE_MAX_H  105.0f
#define ITEM_MAX_W 120.0f
#define ITEM_MAX_H  80.0f
#define PETAL_COUNT 80
#define PETAL_MAX_W 24.0f
#define PETAL_MAX_H 24.0f
#define PETAL_BOOST_MULT  2.0f
#define SNOW_SCALE_MULT 0.55f
#define SEASON_FADE_TIME 0.80f
#define OB_MAX      64
#define IT_MAX      32
#define SPAWN_OB_SEC_MIN 1.10f
#define SPAWN_OB_SEC_MAX 1.85f
#define SPAWN_IT_SEC_MIN 2.20f
#define SPAWN_IT_SEC_MAX 3.40f
#define DOUBLE_TANSU_CHANCE 0.22f
#define DOUBLE_TANSU_GAP    24.0f
#define BOOST_DURATION 3.0f
#define BOOST_MULT     1.35f
#define ITEM_SPAWN_TRIES 10
#define ITEM_SAFE_PAD    6.0f
#define ITEM_X_SHIFT     70.0f
#define FIRE_FLOAT_MIN_Y  240.0f
#define FIRE_FLOAT_MAX_Y  360.0f
#define FIRE_WAVE_AMP_MIN 16.0f
#define FIRE_WAVE_AMP_MAX 34.0f
#define FIRE_WAVE_F_MIN   1.6f
#define FIRE_WAVE_F_MAX   2.5f
#define NONPLAT_MIN_GAP        600.0f
#define NONPLAT_TO_PLAT_GAP    620.0f
#define PLAT_TO_NONPLAT_GAP    420.0f
#define PLAT_TO_PLAT_GAP       260.0f
#define ITEM_ENABLE_METERS 500
#define SCENE_BLACK_FADE_TIME 0.22f
#define JUMP_KEYS_SINGLE() (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_UP) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
#define JUMP_KEYS_GIRL_2P() (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_UP))
#define JUMP_KEYS_BOY_2P()  (IsKeyPressed(KEY_X) || IsKeyPressed(KEY_C) || IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_KP_0))
static float frand_range(float a, float b){ return a + (b - a) * ((float)rand() / (float)RAND_MAX); }
static float fit_scale_up(Texture2D tex, float maxW, float maxH, float upLimit){
    if (tex.width <= 0 || tex.height <= 0) return 1.0f;
    float sx = maxW / (float)tex.width, sy = maxH / (float)tex.height;
    float s = (sx < sy) ? sx : sy;
    if (s > upLimit) s = upLimit;
    return s;
}
static void DrawTextureCover(Texture2D tex, int sw, int sh){
    float texW = (float)tex.width, texH = (float)tex.height;
    float scrW = (float)sw, scrH = (float)sh;
    float texRatio = texW / texH, scrRatio = scrW / scrH;
    Rectangle src = (Rectangle){0, 0, texW, texH};
    Rectangle dst = (Rectangle){0, 0, scrW, scrH};
    if (texRatio > scrRatio){
        float newW = texH * scrRatio;
        src.x = (texW - newW) * 0.5f;
        src.width = newW;
    } else {
        float newH = texW / scrRatio;
        src.y = (texH - newH) * 0.5f;
        src.height = newH;
    }
    DrawTexturePro(tex, src, dst, (Vector2){0,0}, 0.0f, WHITE);
}
static void DrawTexCenter(Texture2D t, float cx, float cy, float scale){
    if (t.id == 0) return;
    Rectangle src = {0, 0, (float)t.width, (float)t.height};
    Rectangle dst = {cx, cy, (float)t.width * scale, (float)t.height * scale};
    Vector2 origin = {dst.width * 0.5f, dst.height * 0.5f};
    DrawTexturePro(t, src, dst, origin, 0.0f, WHITE);
}
static Rectangle ExpandRect(Rectangle r, float pad){ return (Rectangle){ r.x - pad, r.y - pad, r.width + pad*2.0f, r.height + pad*2.0f }; }
static Rectangle MakeHitbox(Rectangle drawRect, float shrinkX, float shrinkY, float offsetX, float offsetY){
    float mx = drawRect.width  * (shrinkX * 0.5f);
    float my = drawRect.height * (shrinkY * 0.5f);
    Rectangle r;
    r.x = drawRect.x + mx + offsetX;
    r.y = drawRect.y + my + offsetY;
    r.width  = drawRect.width  - mx * 2.0f;
    r.height = drawRect.height - my * 2.0f;
    if (r.width < 2) r.width = 2;
    if (r.height < 2) r.height = 2;
    return r;
}
#define JUMPQ_MAX 64
typedef struct{ float t[JUMPQ_MAX]; int head; int tail; } JumpQ;
static void jq_reset(JumpQ *q){ q->head = q->tail = 0; }
static void jq_push(JumpQ *q, float trigDist){
    q->t[q->head] = trigDist;
    q->head = (q->head + 1) % JUMPQ_MAX;
    if (q->head == q->tail) q->tail = (q->tail + 1) % JUMPQ_MAX;
}
static bool jq_peek(JumpQ *q, float *outTrig){
    if (q->tail == q->head) return false;
    *outTrig = q->t[q->tail];
    return true;
}
static void jq_pop(JumpQ *q){
    if (q->tail == q->head) return;
    q->tail = (q->tail + 1) % JUMPQ_MAX;
}
typedef enum{ OB_JAR = 0, OB_TANSU, OB_JICHO, OB_SMOKE, OB_KIND_COUNT } ObKind;
static bool IsPlatformKind(ObKind k){ return (k == OB_TANSU) || (k == OB_JICHO); }
static bool IsNonPlatformKind(ObKind k){ return !IsPlatformKind(k); }
typedef struct{
    bool active;
    ObKind kind;
    float x;
    float feetY;
    Rectangle drawRect;
    Rectangle hitRect;
    float baseTopY;
    float waveAmp;
    float waveFreq;
    float wavePhase;
} Obstacle;
typedef struct{ bool active; float x; float feetY; Rectangle drawRect; Rectangle hitRect; } Item;
typedef struct{
    float x;
    float feetY;
    float vy;
    int jumpCount;
    int standObIndex;
    Rectangle drawRect;
    Rectangle hitRect;
    float scale;
} Player;
typedef struct{ float x, y; float vx, vy; float rot; float rotSpd; float scaleMul; bool active; } Petal;
static bool PlayerDoJump(Player *p, float baseVy){
    if (p->jumpCount >= MAX_JUMP_COUNT) return false;
    float vy = baseVy;
    if (p->jumpCount == 1) vy *= JUMP2_FACTOR;
    p->vy = vy;
    p->jumpCount++;
    p->standObIndex = -1;
    return true;
}
static void ResolvePlatform(Player *p, float prevFeetY, float floorY, Obstacle obs[], int obsCount, float pW){
    if (p->standObIndex >= 0){
        int idx = p->standObIndex;
        if (!(idx >= 0 && idx < obsCount && obs[idx].active && IsPlatformKind(obs[idx].kind))){
            p->standObIndex = -1;
        } else {
            Rectangle o = obs[idx].drawRect;
            float playerL = p->x + pW * 0.15f, playerR = p->x + pW * 0.85f;
            float obL = o.x, obR = o.x + o.width;
            bool overlapX = (playerR > obL) && (playerL < obR);
            if (!overlapX){
                p->standObIndex = -1;
            } else {
                float topY = o.y;
                p->feetY = topY;
                p->vy = 0.0f;
                p->jumpCount = 0;
                return;
            }
        }
    }
    if (p->vy < 0.0f) return;
    int best = -1;
    float bestTopY = -1e9f;
    float playerL = p->x + pW * 0.15f, playerR = p->x + pW * 0.85f;
    for (int i = 0; i < obsCount; i++){
        if (!obs[i].active) continue;
        if (!IsPlatformKind(obs[i].kind)) continue;
        Rectangle o = obs[i].drawRect;
        float topY = o.y;
        float obL = o.x, obR = o.x + o.width;
        bool overlapX = (playerR > obL) && (playerL < obR);
        if (!overlapX) continue;
        float eps = 3.0f;
        if (prevFeetY <= topY + eps && p->feetY >= topY - eps){
            if (topY > bestTopY){ bestTopY = topY; best = i; }
        }
    }
    if (best >= 0){
        p->standObIndex = best;
        p->feetY = bestTopY;
        p->vy = 0.0f;
        p->jumpCount = 0;
        return;
    }
    if (p->feetY > floorY){
        p->feetY = floorY;
        p->vy = 0.0f;
        p->jumpCount = 0;
        p->standObIndex = -1;
    }
}
static bool GetRightmostObstacle(Obstacle obs[], int obsCount, float *outRight, ObKind *outKind){
    float bestRight = -1e9f;
    ObKind bestKind = OB_JAR;
    bool found = false;
    for (int i = 0; i < obsCount; i++){
        if (!obs[i].active) continue;
        float r = obs[i].drawRect.x + obs[i].drawRect.width;
        if (r > bestRight){ bestRight = r; bestKind = obs[i].kind; found = true; }
    }
    if (!found) return false;
    *outRight = bestRight;
    *outKind = bestKind;
    return true;
}
static float EnforceSpawnGap(Obstacle obs[], int obsCount, ObKind newKind, float spawnX){
    float lastRight; ObKind lastKind;
    if (!GetRightmostObstacle(obs, obsCount, &lastRight, &lastKind)) return spawnX;
    bool lastPlat = IsPlatformKind(lastKind);
    bool newPlat  = IsPlatformKind(newKind);
    float reqGap = 0.0f;
    if (!lastPlat && !newPlat) reqGap = NONPLAT_MIN_GAP;
    else if (!lastPlat && newPlat)  reqGap = NONPLAT_TO_PLAT_GAP;
    else if (lastPlat && !newPlat)  reqGap = PLAT_TO_NONPLAT_GAP;
    else reqGap = PLAT_TO_PLAT_GAP;
    float wantX = lastRight + reqGap;
    if (spawnX < wantX) spawnX = wantX;
    return spawnX;
}
static bool HasCloseNonPlatformAhead(Obstacle obs[], int obsCount, float newX, float minGap){
    for (int i = 0; i < obsCount; i++){
        if (!obs[i].active) continue;
        if (!IsNonPlatformKind(obs[i].kind)) continue;
        float prevRight = obs[i].drawRect.x + obs[i].drawRect.width;
        float gap = newX - prevRight;
        if (gap >= 0.0f && gap < minGap) return true;
    }
    return false;
}
static ObKind PickKindBase(void){
    int r = rand() % 100;
    if (r < 22) return OB_JAR;
    else if (r < 52) return OB_TANSU;
    else if (r < 78) return OB_JICHO;
    else return OB_SMOKE;
}
typedef enum{ MODE_SINGLE = 0, MODE_TWO = 1 } GameMode;
typedef enum{ SC_TITLE = 0, SC_PLAY, SC_RESULT } Scene;
typedef struct{ int top[3]; } Ranking;
static void LoadRanking(Ranking *r){
    r->top[0] = r->top[1] = r->top[2] = 0;
    FILE *fp = fopen("ranking.txt", "r");
    if (!fp) return;
    int a=0,b=0,c=0;
    if (fscanf(fp, "%d %d %d", &a, &b, &c) == 3){ r->top[0] = a; r->top[1] = b; r->top[2] = c; }
    fclose(fp);
}
static void SaveRanking(const Ranking *r){
    FILE *fp = fopen("ranking.txt", "w");
    if (!fp) return;
    fprintf(fp, "%d %d %d\n", r->top[0], r->top[1], r->top[2]);
    fclose(fp);
}
static void SortDesc4(int v[4]){
    for (int i = 0; i < 4; i++){
        for (int j = i+1; j < 4; j++){
            if (v[j] > v[i]){ int tmp=v[i]; v[i]=v[j]; v[j]=tmp; }
        }
    }
}
static int InsertScore(Ranking *r, int score){
    int v[4] = { r->top[0], r->top[1], r->top[2], score };
    SortDesc4(v);
    r->top[0] = v[0]; r->top[1] = v[1]; r->top[2] = v[2];
    if (score < r->top[2]) return -1;
    for (int i = 0; i < 3; i++) if (r->top[i] == score) return i;
    return -1;
}
typedef struct{
    Player girl, boy;
    Obstacle obs[OB_MAX];
    Item items[IT_MAX];
    JumpQ qGirl, qBoy;
    float scrollSpeed;
    float dist;
    float boostTime;
    float tSpawnOb;
    float tSpawnIt;
} PlayState;
static void ResetGame(PlayState *ps, float floorY, Texture2D texGirl, Texture2D texBoy){
    ps->girl.x = GIRL_X; ps->girl.feetY = floorY; ps->girl.vy = 0; ps->girl.jumpCount = 0; ps->girl.standObIndex = -1;
    ps->boy.x  = BOY_X;  ps->boy.feetY  = floorY; ps->boy.vy  = 0; ps->boy.jumpCount  = 0; ps->boy.standObIndex  = -1;
    ps->girl.scale = (texGirl.id != 0) ? fit_scale_up(texGirl, P_GIRL_MAX_W, P_GIRL_MAX_H, 10.0f) : 1.0f;
    ps->boy.scale  = (texBoy.id  != 0) ? fit_scale_up(texBoy,  P_BOY_MAX_W,  P_BOY_MAX_H,  10.0f) : 1.0f;
    ps->boy.scale *= BOY_SCALE_BONUS;
    for (int i = 0; i < OB_MAX; i++) ps->obs[i].active = false;
    for (int i = 0; i < IT_MAX; i++) ps->items[i].active = false;
    jq_reset(&ps->qGirl);
    jq_reset(&ps->qBoy);
    ps->scrollSpeed = SPEED_BASE;
    ps->dist = 0.0f;
    ps->boostTime = 0.0f;
    ps->tSpawnOb = frand_range(SPAWN_OB_SEC_MIN, SPAWN_OB_SEC_MAX);
    ps->tSpawnIt = frand_range(SPAWN_IT_SEC_MIN, SPAWN_IT_SEC_MAX);
}
typedef struct{
    bool active;
    float x, y;
    float vx, vy;
    float rot, rotSpd;
    float w, h;
} FlyAnim;
static void StartFly(FlyAnim *f, Rectangle drawRect, bool toLeft){
    f->active = true;
    f->x = drawRect.x + drawRect.width * 0.5f;
    f->y = drawRect.y + drawRect.height * 0.5f;
    f->w = drawRect.width;
    f->h = drawRect.height;
    f->vx = toLeft ? -520.0f : 520.0f;
    f->vy = -980.0f;
    f->rot = 0.0f;
    f->rotSpd = toLeft ? -420.0f : 420.0f;
}
static void UpdateFly(FlyAnim *f, float dt){
    if (!f->active) return;
    f->vy += GRAVITY * 0.85f * dt;
    f->x += f->vx * dt;
    f->y += f->vy * dt;
    f->rot += f->rotSpd * dt;
    if (f->x < -300 || f->x > SCREEN_W + 300 || f->y > SCREEN_H + 400) f->active = false;
}
static void DrawFly(Texture2D tex, const FlyAnim *f){
    if (!f->active || tex.id == 0) return;
    Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
    Rectangle dst = {f->x, f->y, f->w, f->h};
    Vector2 origin = {f->w * 0.5f, f->h * 0.5f};
    DrawTexturePro(tex, src, dst, origin, f->rot, WHITE);
}
static int DesiredParticleIndex(Scene scene, int meters){
    if (scene != SC_PLAY) return 0;
    if (meters < 100) return 0;
    int t = (meters - 100) / 100;
    int cyc = t % 4;
    if (cyc == 0) return 1;
    if (cyc == 1) return 2;
    if (cyc == 2) return 3;
    return 0;
}
int main(void){
    srand((unsigned)time(NULL));
    InitWindow(SCREEN_W, SCREEN_H, "Heian Misurun");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    InitAudioDevice();
    Music bgmSolid   = LoadMusicStream("assets/solid.mp3");
    Music bgmRanking = LoadMusicStream("assets/ranking.mp3");
    Sound seMan   = LoadSound("assets/man.wav");
    Sound seWoman = LoadSound("assets/woman.wav");
    SetMusicVolume(bgmSolid,   0.60f);
    SetMusicVolume(bgmRanking, 0.60f);
    SetSoundVolume(seMan,      0.85f);
    SetSoundVolume(seWoman,    0.85f);
    typedef enum{ BGM_SOLID = 0, BGM_RANKING = 1 } BgmKind;
    int currentBgm = -1;
    Texture2D texBg   = LoadTexture("assets/bg.png");
    Texture2D texSakura = LoadTexture("assets/petal.png");
    Texture2D texLeaf   = LoadTexture("assets/leaf.png");
    Texture2D texMomiji = LoadTexture("assets/momiji.png");
    Texture2D texSnow   = LoadTexture("assets/snow.png");
    Texture2D particleTex[4] = { texSakura, texLeaf, texMomiji, texSnow };
    float particleBaseScale[4];
    for (int i = 0; i < 4; i++) particleBaseScale[i] = (particleTex[i].id != 0) ? fit_scale_up(particleTex[i], PETAL_MAX_W, PETAL_MAX_H, 10.0f) : 1.0f;
    particleBaseScale[3] *= SNOW_SCALE_MULT;
    Texture2D texGirl = LoadTexture("assets/player_lady12.png");
    Texture2D texBoy  = LoadTexture("assets/player_messenger.png");
    Texture2D texJar   = LoadTexture("assets/obs_jar.png");
    Texture2D texTansu = LoadTexture("assets/obs_tansu.png");
    Texture2D texJicho = LoadTexture("assets/obs_jicho.png");
    Texture2D texSmoke = LoadTexture("assets/obs_smoke.png");
    Texture2D texItem = LoadTexture("assets/item_scroll.png");
    Texture2D texTitleLogo   = LoadTexture("assets/title_logo.png");
    Texture2D texTitleLady   = LoadTexture("assets/title_lady12.png");
    Texture2D texTitleMsg    = LoadTexture("assets/title_messenger.png");
    Texture2D texBtnSingle   = LoadTexture("assets/ui_btn_single.png");
    Texture2D texBtnTwo      = LoadTexture("assets/ui_btn_two.png");
    Texture2D texCursor      = LoadTexture("assets/ui_cursor.png");
    Texture2D texHint        = LoadTexture("assets/ui_hint.png");
    Texture2D obTex[OB_KIND_COUNT] = { texJar, texTansu, texJicho, texSmoke };
    float obMaxW[OB_KIND_COUNT] = { JAR_MAX_W, TANSU_MAX_W, JICHO_MAX_W, SMOKE_MAX_W };
    float obMaxH[OB_KIND_COUNT] = { JAR_MAX_H, TANSU_MAX_H, JICHO_MAX_H, SMOKE_MAX_H };
    float obShrinkX[OB_KIND_COUNT] = { 0.20f, 0.08f, 0.10f, 0.38f };
    float obShrinkY[OB_KIND_COUNT] = { 0.14f, 0.35f, 0.25f, 0.38f };
    float floorY = (float)GetScreenHeight() * FLOOR_Y_RATIO;
    PlayState ps;
    Petal petals[PETAL_COUNT];
    Scene scene = SC_TITLE;
    GameMode mode = MODE_SINGLE;
    int menuSel = 0;
    bool debugHit = false;
    Ranking ranking;
    LoadRanking(&ranking);
    int lastMeters = 0;
    int lastRankIndex = -1;
    FlyAnim flyGirl = (FlyAnim){0};
    FlyAnim flyBoy  = (FlyAnim){0};
    bool deadGirl = false;
    bool deadBoy  = false;
    float sceneBlack = 1.0f;
    #define SCENE_FADE_START() do { sceneBlack = 1.0f; } while(0)
    int seasonCurIdx = 0;
    int seasonNextIdx = 0;
    bool seasonFading = false;
    float seasonBlend = 0.0f;
    for (int i = 0; i < PETAL_COUNT; i++){
        petals[i].active = true;
        petals[i].x = frand_range(0, (float)SCREEN_W);
        petals[i].y = frand_range(0, (float)SCREEN_H);
        petals[i].vx = frand_range(-30.0f, -90.0f);
        petals[i].vy = frand_range(20.0f, 80.0f);
        petals[i].rot = frand_range(0, 360.0f);
        petals[i].rotSpd = frand_range(-90.0f, 90.0f);
        petals[i].scaleMul = frand_range(0.80f, 1.25f);
    }
    ResetGame(&ps, floorY, texGirl, texBoy);
    while (!WindowShouldClose()){
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;
        float now = (float)GetTime();
        if (IsKeyPressed(KEY_H)) debugHit = !debugHit;
        int wantBgm = (scene == SC_RESULT) ? BGM_RANKING : BGM_SOLID;
        if (wantBgm != currentBgm){
            StopMusicStream(bgmSolid);
            StopMusicStream(bgmRanking);
            if (wantBgm == BGM_SOLID) PlayMusicStream(bgmSolid);
            else PlayMusicStream(bgmRanking);
            currentBgm = wantBgm;
        }
        if (currentBgm == BGM_SOLID) UpdateMusicStream(bgmSolid);
        else if (currentBgm == BGM_RANKING) UpdateMusicStream(bgmRanking);
        if (sceneBlack > 0.0f){
            sceneBlack -= dt / SCENE_BLACK_FADE_TIME;
            if (sceneBlack < 0.0f) sceneBlack = 0.0f;
        }
        int metersNow = (int)(ps.dist / 50.0f);
        int desiredIdx = DesiredParticleIndex(scene, metersNow);
        if (scene != SC_PLAY){
            seasonCurIdx = 0;
            seasonNextIdx = 0;
            seasonFading = false;
            seasonBlend = 0.0f;
        } else {
            if (!seasonFading){
                if (desiredIdx != seasonCurIdx){
                    seasonNextIdx = desiredIdx;
                    seasonBlend = 0.0f;
                    seasonFading = true;
                }
            } else {
                seasonBlend += dt / SEASON_FADE_TIME;
                if (seasonBlend >= 1.0f){
                    seasonCurIdx = seasonNextIdx;
                    seasonBlend = 0.0f;
                    seasonFading = false;
                }
            }
        }
        Texture2D curA = particleTex[seasonCurIdx];
        float scaleA = particleBaseScale[seasonCurIdx];
        Texture2D curB = particleTex[seasonNextIdx];
        float scaleB = particleBaseScale[seasonNextIdx];
        float petMul = ((scene == SC_PLAY && ps.boostTime > 0.0f) ? PETAL_BOOST_MULT : 1.0f);
        for (int i = 0; i < PETAL_COUNT; i++){
            if (!petals[i].active) continue;
            petals[i].x += petals[i].vx * dt * petMul;
            petals[i].y += petals[i].vy * dt * petMul;
            petals[i].rot += petals[i].rotSpd * dt * petMul;
            if (petals[i].x < -50) petals[i].x = (float)GetScreenWidth() + frand_range(0, 200);
            if (petals[i].y > (float)GetScreenHeight() + 50) petals[i].y = -frand_range(0, 200);
        }
        if (scene == SC_TITLE){
            if (IsKeyPressed(KEY_ONE)) menuSel = 0;
            if (IsKeyPressed(KEY_TWO)) menuSel = 1;
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) menuSel = 0;
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) menuSel = 1;
            float sBtn = (texBtnSingle.id != 0) ? fit_scale_up(texBtnSingle, SCREEN_W * 0.52f, SCREEN_H * 0.20f, 20.0f) : 1.0f;
            sBtn *= 1.40f;
            float btnW = (texBtnSingle.id != 0) ? texBtnSingle.width * sBtn : 420.0f;
            float btnH = (texBtnSingle.id != 0) ? texBtnSingle.height * sBtn : 140.0f;
            float cx = SCREEN_W * 0.50f;
            float y1 = SCREEN_H * 0.42f;
            float y2 = SCREEN_H * 0.75f;
            Rectangle rSingle = { cx - btnW*0.5f, y1 - btnH*0.5f, btnW, btnH };
            Rectangle rTwo    = { cx - btnW*0.5f, y2 - btnH*0.5f, btnW, btnH };
            Vector2 m = GetMousePosition();
            if (CheckCollisionPointRec(m, rSingle)) menuSel = 0;
            if (CheckCollisionPointRec(m, rTwo))    menuSel = 1;
            bool start = false;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) start = true;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
                if (CheckCollisionPointRec(m, rSingle) || CheckCollisionPointRec(m, rTwo)) start = true;
            }
            if (start){
                mode = (menuSel == 0) ? MODE_SINGLE : MODE_TWO;
                ResetGame(&ps, floorY, texGirl, texBoy);
                flyGirl.active = false;
                flyBoy.active  = false;
                deadGirl = false;
                deadBoy  = false;
                SCENE_FADE_START();
                scene = SC_PLAY;
            }
        } else if (scene == SC_PLAY){
            if (IsKeyPressed(KEY_ESCAPE)){ scene = SC_TITLE; SCENE_FADE_START(); }
            bool boosting = (ps.boostTime > 0.0f);
            float moveSpeed = ps.scrollSpeed;
            if (boosting){
                ps.boostTime -= dt;
                if (ps.boostTime < 0.0f) ps.boostTime = 0.0f;
                moveSpeed *= BOOST_MULT;
            }
            ps.scrollSpeed += SPEED_UP * dt;
            ps.dist += moveSpeed * dt;
            metersNow = (int)(ps.dist / 50.0f);
            float girlW = (texGirl.id != 0) ? (float)texGirl.width * ps.girl.scale : 120.0f;
            float girlH = (texGirl.id != 0) ? (float)texGirl.height * ps.girl.scale : 120.0f;
            float boyW  = (texBoy.id  != 0) ? (float)texBoy.width  * ps.boy.scale  : 120.0f;
            float boyH  = (texBoy.id  != 0) ? (float)texBoy.height * ps.boy.scale  : 120.0f;
            float prevFeetYG = ps.girl.feetY;
            float prevFeetYB = ps.boy.feetY;
            if (mode == MODE_SINGLE){
                if (JUMP_KEYS_SINGLE()){
                    float gap = fabsf(ps.girl.x - ps.boy.x);
                    if (ps.girl.x >= ps.boy.x){
                        jq_push(&ps.qGirl, ps.dist);
                        jq_push(&ps.qBoy,  ps.dist + gap);
                    } else {
                        jq_push(&ps.qBoy,  ps.dist);
                        jq_push(&ps.qGirl, ps.dist + gap);
                    }
                }
                float trig = 0.0f;
                while (jq_peek(&ps.qGirl, &trig) && trig <= ps.dist){
                    if (PlayerDoJump(&ps.girl, JUMP_VY_GIRL)) PlaySound(seWoman);
                    jq_pop(&ps.qGirl);
                }
                while (jq_peek(&ps.qBoy, &trig) && trig <= ps.dist){
                    if (PlayerDoJump(&ps.boy, JUMP_VY_BOY)) PlaySound(seMan);
                    jq_pop(&ps.qBoy);
                }
            } else {
                if (JUMP_KEYS_GIRL_2P()) if (PlayerDoJump(&ps.girl, JUMP_VY_GIRL)) PlaySound(seWoman);
                if (JUMP_KEYS_BOY_2P())  if (PlayerDoJump(&ps.boy,  JUMP_VY_BOY))  PlaySound(seMan);
            }
            ps.girl.vy += GRAVITY * dt;
            ps.boy.vy  += GRAVITY * dt;
            ps.girl.feetY += ps.girl.vy * dt;
            ps.boy.feetY  += ps.boy.vy  * dt;
            ps.tSpawnOb -= dt;
            ps.tSpawnIt -= dt;
            if (ps.tSpawnOb <= 0.0f){
                ObKind k = PickKindBase();
                float spawnX = (float)GetScreenWidth() + 80.0f;
                spawnX = EnforceSpawnGap(ps.obs, OB_MAX, k, spawnX);
                if (IsNonPlatformKind(k) && HasCloseNonPlatformAhead(ps.obs, OB_MAX, spawnX, NONPLAT_MIN_GAP)){
                    k = (rand() % 2 == 0) ? OB_TANSU : OB_JICHO;
                    spawnX = EnforceSpawnGap(ps.obs, OB_MAX, k, spawnX);
                }
                for (int i = 0; i < OB_MAX; i++){
                    if (!ps.obs[i].active){
                        Texture2D t = obTex[k];
                        float s = (t.id != 0) ? fit_scale_up(t, obMaxW[k], obMaxH[k], 10.0f) : 1.0f;
                        float w = (t.id != 0) ? (float)t.width * s : obMaxW[k];
                        float h = (t.id != 0) ? (float)t.height * s : obMaxH[k];
                        ps.obs[i].active = true;
                        ps.obs[i].kind = k;
                        ps.obs[i].x = spawnX;
                        ps.obs[i].feetY = floorY;
                        if (k == OB_SMOKE){
                            float up = frand_range(FIRE_FLOAT_MIN_Y, FIRE_FLOAT_MAX_Y);
                            ps.obs[i].baseTopY  = (floorY - up) - h * 0.5f;
                            ps.obs[i].waveAmp   = frand_range(FIRE_WAVE_AMP_MIN, FIRE_WAVE_AMP_MAX);
                            ps.obs[i].waveFreq  = frand_range(FIRE_WAVE_F_MIN, FIRE_WAVE_F_MAX);
                            ps.obs[i].wavePhase = frand_range(0.0f, 6.28318f);
                            ps.obs[i].drawRect = (Rectangle){ ps.obs[i].x, ps.obs[i].baseTopY, w, h };
                        } else {
                            ps.obs[i].drawRect = (Rectangle){ ps.obs[i].x, ps.obs[i].feetY - h, w, h };
                            ps.obs[i].baseTopY  = ps.obs[i].drawRect.y;
                            ps.obs[i].waveAmp   = 0.0f;
                            ps.obs[i].waveFreq  = 0.0f;
                            ps.obs[i].wavePhase = 0.0f;
                        }
                        ps.obs[i].hitRect = MakeHitbox(ps.obs[i].drawRect, obShrinkX[k], obShrinkY[k], 0, 0);
                        if (k == OB_TANSU){
                            float rr = (float)rand() / (float)RAND_MAX;
                            if (rr < DOUBLE_TANSU_CHANCE){
                                for (int j = 0; j < OB_MAX; j++){
                                    if (!ps.obs[j].active){
                                        ps.obs[j] = ps.obs[i];
                                        ps.obs[j].x = ps.obs[i].x + ps.obs[i].drawRect.width + DOUBLE_TANSU_GAP;
                                        ps.obs[j].drawRect.x = ps.obs[j].x;
                                        ps.obs[j].hitRect = MakeHitbox(ps.obs[j].drawRect, obShrinkX[OB_TANSU], obShrinkY[OB_TANSU], 0, 0);
                                        break;
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
                ps.tSpawnOb = frand_range(SPAWN_OB_SEC_MIN, SPAWN_OB_SEC_MAX);
            }
            if (ps.tSpawnIt <= 0.0f){
                if (metersNow < ITEM_ENABLE_METERS){
                    ps.tSpawnIt = 0.35f;
                } else {
                    for (int i = 0; i < IT_MAX; i++){
                        if (ps.items[i].active) continue;
                        float s = (texItem.id != 0) ? fit_scale_up(texItem, ITEM_MAX_W, ITEM_MAX_H, 10.0f) : 1.0f;
                        float w = (texItem.id != 0) ? (float)texItem.width * s : ITEM_MAX_W;
                        float h = (texItem.id != 0) ? (float)texItem.height * s : ITEM_MAX_H;
                        for (int tr = 0; tr < ITEM_SPAWN_TRIES; tr++){
                            float x = (float)GetScreenWidth() + 120.0f + (float)tr * ITEM_X_SHIFT;
                            float feetY2 = floorY - frand_range(80.0f, 140.0f);
                            Rectangle drawR = (Rectangle){ x, feetY2 - h, w, h };
                            Rectangle hitR  = MakeHitbox(drawR, 0.35f, 0.35f, 0, 0);
                            Rectangle safeR = ExpandRect(drawR, ITEM_SAFE_PAD);
                            bool ok = true;
                            for (int j = 0; j < OB_MAX; j++){
                                if (!ps.obs[j].active) continue;
                                if (CheckCollisionRecs(safeR, ps.obs[j].drawRect)){ ok = false; break; }
                            }
                            if (ok){
                                for (int j = 0; j < IT_MAX; j++){
                                    if (!ps.items[j].active) continue;
                                    if (CheckCollisionRecs(safeR, ps.items[j].drawRect)){ ok = false; break; }
                                }
                            }
                            if (ok){
                                ps.items[i].active = true;
                                ps.items[i].x = x;
                                ps.items[i].feetY = feetY2;
                                ps.items[i].drawRect = drawR;
                                ps.items[i].hitRect  = hitR;
                                break;
                            }
                        }
                        break;
                    }
                    ps.tSpawnIt = frand_range(SPAWN_IT_SEC_MIN, SPAWN_IT_SEC_MAX);
                }
            }
            for (int i = 0; i < OB_MAX; i++){
                if (!ps.obs[i].active) continue;
                ps.obs[i].x -= moveSpeed * dt;
                ps.obs[i].drawRect.x = ps.obs[i].x;
                ObKind k = ps.obs[i].kind;
                if (k == OB_SMOKE){
                    float y = ps.obs[i].baseTopY + sinf(now * ps.obs[i].waveFreq + ps.obs[i].wavePhase) * ps.obs[i].waveAmp;
                    ps.obs[i].drawRect.y = y;
                }
                ps.obs[i].hitRect = MakeHitbox(ps.obs[i].drawRect, obShrinkX[k], obShrinkY[k], 0, 0);
                if (ps.obs[i].drawRect.x + ps.obs[i].drawRect.width < -50) ps.obs[i].active = false;
            }
            for (int i = 0; i < IT_MAX; i++){
                if (!ps.items[i].active) continue;
                ps.items[i].x -= moveSpeed * dt;
                ps.items[i].drawRect.x = ps.items[i].x;
                ps.items[i].hitRect = MakeHitbox(ps.items[i].drawRect, 0.35f, 0.35f, 0, 0);
                if (ps.items[i].drawRect.x + ps.items[i].drawRect.width < -50) ps.items[i].active = false;
            }
            ResolvePlatform(&ps.girl, prevFeetYG, floorY, ps.obs, OB_MAX, girlW);
            ResolvePlatform(&ps.boy,  prevFeetYB, floorY, ps.obs, OB_MAX, boyW);
            ps.girl.drawRect = (Rectangle){ ps.girl.x, ps.girl.feetY - girlH, girlW, girlH };
            ps.girl.hitRect  = MakeHitbox(ps.girl.drawRect, 0.55f, 0.30f, girlW * 0.10f, girlH * 0.05f);
            ps.boy.drawRect = (Rectangle){ ps.boy.x, ps.boy.feetY - boyH, boyW, boyH };
            ps.boy.hitRect  = MakeHitbox(ps.boy.drawRect, 0.55f, 0.30f, boyW * 0.10f, boyH * 0.05f);
            bool hitG_any = false;
            bool hitB_any = false;
            for (int i = 0; i < OB_MAX; i++){
                if (!ps.obs[i].active) continue;
                if (IsPlatformKind(ps.obs[i].kind)){
                    bool hitG = (ps.girl.standObIndex != i) && CheckCollisionRecs(ps.girl.hitRect, ps.obs[i].hitRect);
                    bool hitB = (ps.boy.standObIndex  != i) && CheckCollisionRecs(ps.boy.hitRect,  ps.obs[i].hitRect);
                    if (hitG) hitG_any = true;
                    if (hitB) hitB_any = true;
                } else {
                    if (CheckCollisionRecs(ps.girl.hitRect, ps.obs[i].hitRect)) hitG_any = true;
                    if (CheckCollisionRecs(ps.boy.hitRect,  ps.obs[i].hitRect)) hitB_any = true;
                }
                if (hitG_any || hitB_any) break;
            }
            for (int i = 0; i < IT_MAX; i++){
                if (!ps.items[i].active) continue;
                if (CheckCollisionRecs(ps.girl.hitRect, ps.items[i].hitRect) || CheckCollisionRecs(ps.boy.hitRect, ps.items[i].hitRect)){
                    ps.items[i].active = false;
                    ps.boostTime = BOOST_DURATION;
                    break;
                }
            }
            if (hitG_any || hitB_any){
                lastMeters = (int)(ps.dist / 50.0f);
                bool causeGirl = hitG_any;
                bool causeBoy  = hitB_any;
                if (hitG_any && hitB_any){
                    if (ps.girl.x >= ps.boy.x) causeBoy = false;
                    else causeGirl = false;
                }
                deadGirl = causeGirl;
                deadBoy  = causeBoy;
                flyGirl.active = false;
                flyBoy.active  = false;
                if (deadGirl) StartFly(&flyGirl, ps.girl.drawRect, false);
                if (deadBoy)  StartFly(&flyBoy,  ps.boy.drawRect,  true);
                lastRankIndex = InsertScore(&ranking, lastMeters);
                SaveRanking(&ranking);
                scene = SC_RESULT;
                SCENE_FADE_START();
            }
        } else if (scene == SC_RESULT){
            UpdateFly(&flyGirl, dt);
            UpdateFly(&flyBoy,  dt);
            if (IsKeyPressed(KEY_R)){
                ResetGame(&ps, floorY, texGirl, texBoy);
                flyGirl.active = false;
                flyBoy.active  = false;
                deadGirl = false;
                deadBoy  = false;
                scene = SC_PLAY;
                SCENE_FADE_START();
            }
            if (IsKeyPressed(KEY_ESCAPE)){ scene = SC_TITLE; SCENE_FADE_START(); }
        }
        BeginDrawing();
        if (texBg.id != 0) DrawTextureCover(texBg, GetScreenWidth(), GetScreenHeight());
        else ClearBackground(BLACK);
        if (curA.id != 0){
            float aAlpha = 1.0f, bAlpha = 0.0f;
            if (seasonFading){ aAlpha = 1.0f - seasonBlend; bAlpha = seasonBlend; }
            Color colA = Fade(WHITE, aAlpha);
            Color colB = Fade(WHITE, bAlpha);
            for (int i = 0; i < PETAL_COUNT; i++){
                if (!petals[i].active) continue;
                if (aAlpha > 0.001f){
                    float sA = scaleA * petals[i].scaleMul;
                    float wA = (float)curA.width * sA;
                    float hA = (float)curA.height * sA;
                    Rectangle srcA = {0, 0, (float)curA.width, (float)curA.height};
                    Rectangle dstA = {petals[i].x, petals[i].y, wA, hA};
                    Vector2 originA = {wA * 0.5f, hA * 0.5f};
                    DrawTexturePro(curA, srcA, dstA, originA, petals[i].rot, colA);
                }
                if (seasonFading && bAlpha > 0.001f && curB.id != 0){
                    float sB = scaleB * petals[i].scaleMul;
                    float wB = (float)curB.width * sB;
                    float hB = (float)curB.height * sB;
                    Rectangle srcB = {0, 0, (float)curB.width, (float)curB.height};
                    Rectangle dstB = {petals[i].x, petals[i].y, wB, hB};
                    Vector2 originB = {wB * 0.5f, hB * 0.5f};
                    DrawTexturePro(curB, srcB, dstB, originB, petals[i].rot, colB);
                }
            }
        }
        if (scene == SC_TITLE){
            if (texTitleLogo.id != 0){
                float sLogo = fit_scale_up(texTitleLogo, SCREEN_W * 0.92f, SCREEN_H * 0.32f, 20.0f);
                sLogo *= 1.10f;
                DrawTexCenter(texTitleLogo, SCREEN_W * 0.50f, SCREEN_H * 0.15f, sLogo);
            }
            if (texTitleMsg.id != 0){
                float s = fit_scale_up(texTitleMsg, SCREEN_W * 0.34f, SCREEN_H * 0.78f, 20.0f);
                s *= TITLE_BOY_SCALE_BONUS;
                float w = texTitleMsg.width * s;
                float h = texTitleMsg.height * s;
                Rectangle src = {0,0,(float)texTitleMsg.width,(float)texTitleMsg.height};
                Rectangle dst = { 40, floorY - h + 20, w, h };
                DrawTexturePro(texTitleMsg, src, dst, (Vector2){0,0}, 0.0f, WHITE);
            }
            if (texTitleLady.id != 0){
                float s = fit_scale_up(texTitleLady, SCREEN_W * 0.34f, SCREEN_H * 0.78f, 20.0f);
                float w = texTitleLady.width * s;
                float h = texTitleLady.height * s;
                Rectangle src = {0,0,(float)texTitleLady.width,(float)texTitleLady.height};
                Rectangle dst = { SCREEN_W - w - 40, floorY - h + 20, w, h };
                DrawTexturePro(texTitleLady, src, dst, (Vector2){0,0}, 0.0f, WHITE);
            }
            float sBtn = (texBtnSingle.id != 0) ? fit_scale_up(texBtnSingle, SCREEN_W * 0.52f, SCREEN_H * 0.20f, 20.0f) : 1.0f;
            sBtn *= 1.40f;
            float btnW = (texBtnSingle.id != 0) ? texBtnSingle.width * sBtn : 420.0f;
            float btnH = (texBtnSingle.id != 0) ? texBtnSingle.height * sBtn : 140.0f;
            float cx = SCREEN_W * 0.50f;
            float y1 = SCREEN_H * 0.42f;
            float y2 = SCREEN_H * 0.75f;
            Rectangle hi = { cx - btnW*0.5f - 10, (menuSel==0? y1:y2) - btnH*0.5f - 10, btnW + 20, btnH + 20 };
            DrawRectangleRounded(hi, 0.12f, 12, (Color){255,255,255,35});
            if (texBtnSingle.id != 0) DrawTexCenter(texBtnSingle, cx, y1, sBtn);
            if (texBtnTwo.id != 0)    DrawTexCenter(texBtnTwo,    cx, y2, sBtn);
            if (texCursor.id != 0){
                float sCur = fit_scale_up(texCursor, 70, 70, 20.0f);
                float curW = texCursor.width * sCur;
                float curX = cx - btnW*0.5f - curW*0.75f;
                float curY = (menuSel==0)? y1 : y2;
                DrawTexCenter(texCursor, curX, curY, sCur);
            }
            if (texHint.id != 0){
                float sHint = fit_scale_up(texHint, SCREEN_W * 0.36f, SCREEN_H * 0.28f, 20.0f);
                float w = texHint.width * sHint;
                float h = texHint.height * sHint;
                Rectangle src = {0,0,(float)texHint.width,(float)texHint.height};
                Rectangle dst = { SCREEN_W - w - 40, SCREEN_H - h - 30, w, h };
                DrawTexturePro(texHint, src, dst, (Vector2){0,0}, 0.0f, WHITE);
            }
        } else {
            for (int i = 0; i < OB_MAX; i++){
                if (!ps.obs[i].active) continue;
                Texture2D t = obTex[ps.obs[i].kind];
                if (t.id == 0) continue;
                Rectangle src = {0, 0, (float)t.width, (float)t.height};
                DrawTexturePro(t, src, ps.obs[i].drawRect, (Vector2){0, 0}, 0.0f, WHITE);
                if (debugHit) DrawRectangleLinesEx(ps.obs[i].hitRect, 2, RED);
            }
            if (texItem.id != 0){
                for (int i = 0; i < IT_MAX; i++){
                    if (!ps.items[i].active) continue;
                    Rectangle src = {0, 0, (float)texItem.width, (float)texItem.height};
                    DrawTexturePro(texItem, src, ps.items[i].drawRect, (Vector2){0, 0}, 0.0f, WHITE);
                    if (debugHit) DrawRectangleLinesEx(ps.items[i].hitRect, 2, BLUE);
                }
            }
            if (scene == SC_RESULT){
                if (deadGirl){
                    if (flyGirl.active) DrawFly(texGirl, &flyGirl);
                } else {
                    if (texGirl.id != 0){
                        Rectangle src = {0, 0, (float)texGirl.width, (float)texGirl.height};
                        DrawTexturePro(texGirl, src, ps.girl.drawRect, (Vector2){0, 0}, 0.0f, WHITE);
                    }
                }
                if (deadBoy){
                    if (flyBoy.active) DrawFly(texBoy, &flyBoy);
                } else {
                    if (texBoy.id != 0){
                        Rectangle src = {0, 0, (float)texBoy.width, (float)texBoy.height};
                        DrawTexturePro(texBoy, src, ps.boy.drawRect, (Vector2){0, 0}, 0.0f, WHITE);
                    }
                }
            } else {
                if (texGirl.id != 0){
                    Rectangle src = {0, 0, (float)texGirl.width, (float)texGirl.height};
                    DrawTexturePro(texGirl, src, ps.girl.drawRect, (Vector2){0, 0}, 0.0f, WHITE);
                }
                if (texBoy.id != 0){
                    Rectangle src = {0, 0, (float)texBoy.width, (float)texBoy.height};
                    DrawTexturePro(texBoy, src, ps.boy.drawRect, (Vector2){0, 0}, 0.0f, WHITE);
                }
            }
            if (debugHit){
                DrawRectangleLinesEx(ps.girl.hitRect, 2, GREEN);
                DrawRectangleLinesEx(ps.boy.hitRect,  2, GREEN);
                DrawLine(0, (int)floorY, GetScreenWidth(), (int)floorY, YELLOW);
            }
            int meters = (int)(ps.dist / 50.0f);
            DrawText(TextFormat("DIST: %05d m", meters), 30, 20, 36, RAYWHITE);
            if (ps.boostTime > 0.0f){
                DrawText(TextFormat("BOOST! %.1fs", ps.boostTime), 30, 60, 30, (Color){255, 230, 120, 255});
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){255, 200, 100, 20});
            }
            if (scene == SC_RESULT){
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0,0,0,140});
                bool blinkOn = (sinf(now * 6.0f) > 0.0f);
                const char *go = "GAME OVER";
                int w1 = MeasureText(go, 60);
                DrawText(go, (GetScreenWidth()-w1)/2, GetScreenHeight()/2 - 210, 60, RAYWHITE);
                const char *rk = "RANKING";
                int wr = MeasureText(rk, 36);
                DrawText(rk, (GetScreenWidth()-wr)/2, GetScreenHeight()/2 - 140, 36, RAYWHITE);
                int baseY = GetScreenHeight()/2 - 90;
                for (int i = 0; i < 3; i++){
                    bool hi = (lastRankIndex == i);
                    Color col = RAYWHITE;
                    if (hi) col = blinkOn ? (Color){255, 230, 120, 255} : (Color){255, 230, 120, 70};
                    DrawText(TextFormat("%d.  %d m", i+1, ranking.top[i]), GetScreenWidth()/2 - 120, baseY + i*38, 30, col);
                }
                if (lastRankIndex < 0){
                    Color c = blinkOn ? (Color){255, 230, 120, 255} : (Color){255, 230, 120, 70};
                    DrawText(TextFormat("YOUR SCORE: %d m", lastMeters), GetScreenWidth()/2 - 160, baseY + 3*38 + 18, 28, c);
                }
                DrawText("Press R to Retry", GetScreenWidth()/2 - 130, GetScreenHeight()/2 + 120, 28, RAYWHITE);
                DrawText("Esc to Title",     GetScreenWidth()/2 - 105, GetScreenHeight()/2 + 155, 28, RAYWHITE);
            }
        }
        if (sceneBlack > 0.0f) DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, sceneBlack));
        EndDrawing();
    }
    UnloadTexture(texBg);
    UnloadTexture(texSakura);
    UnloadTexture(texLeaf);
    UnloadTexture(texMomiji);
    UnloadTexture(texSnow);
    UnloadTexture(texGirl);
    UnloadTexture(texBoy);
    UnloadTexture(texJar);
    UnloadTexture(texTansu);
    UnloadTexture(texJicho);
    UnloadTexture(texSmoke);
    UnloadTexture(texItem);
    UnloadTexture(texTitleLogo);
    UnloadTexture(texTitleLady);
    UnloadTexture(texTitleMsg);
    UnloadTexture(texBtnSingle);
    UnloadTexture(texBtnTwo);
    UnloadTexture(texCursor);
    UnloadTexture(texHint);
    UnloadSound(seMan);
    UnloadSound(seWoman);
    UnloadMusicStream(bgmSolid);
    UnloadMusicStream(bgmRanking);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
