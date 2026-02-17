#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define GRID 8
#define TILE 96
#define PREVIEW_SIZE 128
#define MAX_ENEMIES 1024
#define HAND_SIZE 4
#define STATUS_BAR_SIZE 112
#define MAX_DAMAGE_EVENTS 32
#define DAMAGE_LINGER_DURATION 1.2f
#define MAX_PASS_COUNTER 5

int spaceCounter = 0;
int sirkcontrols = 0;
int luckcontrols = 1;
const char* interruptMessage;

typedef enum {GAME_SPLASH, GAME_PLAYING} GameState;
GameState gameState = GAME_SPLASH;

typedef enum {TILE_EMPTY, TILE_WALL, TILE_PILLAR} TileType;
typedef enum {
    ENEMY_VAMPIRE,
    ENEMY_SKELETON,
    ENEMY_WEAPON,
    ENEMY_CRATE,
    ENEMY_TROLL,
    ENEMY_VINES,
    ENEMY_CHEST,
    ENEMY_FOOD,
    ENEMY_POTION,
    ENEMY_NONE
} EnemyType;
typedef struct {
    int x, y;
    int amount;
    float timer;
    bool active;
    bool isPlayer;
} DamageEvent;
#define DrawText(text, posX, posY, fontSize, color) DrawTextEx(gameFont, text, (Vector2){(float)(posX), (float)(posY)}, (float)(fontSize), 2, color)
#define MeasureText(text, fontSize) MeasureTextEx(gameFont, text, (float)(fontSize), 2).x
DamageEvent damageEvents[MAX_DAMAGE_EVENTS];

void DrawScaled(Texture2D tex, int x, int y, int size) {
    Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
    Rectangle dst = {(float)x, (float)y, (float)size, (float)size};
    Vector2 origin = {0, 0};
    DrawTexturePro(tex, src, dst, origin, 0.0f, WHITE);
}
void DrawScaledDistorted(Texture2D tex, int x, int y, int size, int sizey) {
    Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
    Rectangle dst = {(float)x, (float)y, (float)size, (float)sizey};
    Vector2 origin = {0, 0};
    DrawTexturePro(tex, src, dst, origin, 0.0f, WHITE);
}

typedef struct {
    int x, y;
    int hp;
    int atk;
    int treasure;
    int food;
    bool alive;
    bool canMove;
    EnemyType type;
    float hitTimer;  
} Enemy;

typedef struct {
    int x, y;
    int hp, atk, stamina, mana, treasure, fun;
    float hitTimer;  
} Player;

TileType grid[GRID][GRID];
Enemy enemies[MAX_ENEMIES];
int enemyCount = 0;
EnemyType hand[HAND_SIZE];
Player player;
Texture2D texPlayer, texVampire, texSkeleton, texPotion, texTroll, texCrate, texVines, texChest, texWall, texFloor, texPillar, texHP, texST, texMP, texTR, texATT, texCard;
Font gameFont;

void SpawnDamage(int x, int y, int amount, bool isPlayer) {
    for(int i = 0; i < MAX_DAMAGE_EVENTS; i++) {
        if(damageEvents[i].active) continue;
        damageEvents[i].active = true;
        damageEvents[i].x = x;
        damageEvents[i].y = y;
        damageEvents[i].amount = amount;
        damageEvents[i].timer = DAMAGE_LINGER_DURATION;
        damageEvents[i].isPlayer = isPlayer;
        break;
    }
}

bool Inside(int x, int y) { return x >= 0 && y >= 0 && x < GRID && y < GRID; }

Enemy *EnemyAt(int x, int y) {
    for(int i = 0; i < enemyCount; i++)
        if(enemies[i].alive && enemies[i].x == x && enemies[i].y == y)
            return &enemies[i];
    return NULL;
}

bool EmptyTile(int x, int y) {
    if(!Inside(x, y)) return false;
    if(grid[y][x] == TILE_WALL || grid[y][x] == TILE_PILLAR) return false;
    if(EnemyAt(x, y)) return false;
    if(player.x == x && player.y == y) return false;
    return true;
}

Enemy enemyTemplates[] = {
    {0,0, 8,3,2, 0, true,  true,  ENEMY_VAMPIRE},
    {0,0, 5,5,2, 0, true,  true,  ENEMY_SKELETON},
    {0,0, 1,0,0,-1, true,  false, ENEMY_WEAPON},
    {0,0, 5,0,0, 0, true,  false, ENEMY_CRATE},
    {0,0,12,2,0, 0, true,  true,  ENEMY_TROLL},
    {0,0, 5,2,0, 5, true,  false, ENEMY_VINES},
    {0,0,20,0,5, 0, true,  false, ENEMY_CHEST},
    {0,0, 1,0,0, 5, true,  false, ENEMY_FOOD},
    {0,0, 1,-3,0,0, true,  false, ENEMY_POTION},
};

Enemy MakeEnemy(EnemyType t, int x, int y) {
    Enemy e = enemyTemplates[t];
    e.x = x;
    e.y = y;
    e.alive = true;
    e.hitTimer = 0.0f;
    return e;
}

EnemyType RandomEnemyType() { return static_cast<EnemyType>(GetRandomValue(0, ENEMY_NONE-1)); }

void InitGrid() {
    for(int y = 0; y < GRID; y++)
        for(int x = 0; x < GRID; x++) {
            if(x == 0 || y == 0 || x == GRID - 1 || y == GRID - 1) grid[y][x] = TILE_WALL;
            else grid[y][x] = TILE_EMPTY;
        }
}

void DrawTileBack(int x, int y)
{
    if(grid[y][x] == TILE_WALL) return;
    else if(grid[y][x] == TILE_PILLAR) return;
    DrawScaled(texFloor, x * TILE, y * TILE + STATUS_BAR_SIZE, TILE);
}
void DrawTileFront(int x, int y)
{
    if(grid[y][x] == TILE_WALL)
        DrawScaledDistorted(texWall, x * TILE, y * TILE - TILE / 2 + STATUS_BAR_SIZE, TILE, TILE * 3 / 2);
    else if(grid[y][x] == TILE_PILLAR) {
        DrawScaled(texFloor, x * TILE, y * TILE + STATUS_BAR_SIZE, TILE);
        DrawScaledDistorted(texPillar, x * TILE, y * TILE - TILE / 2 + STATUS_BAR_SIZE, TILE, TILE * 3 / 2);
    }
}

Texture2D GetEnemyTexture(EnemyType t) {
    if(t == ENEMY_VAMPIRE) return texVampire;
    if(t == ENEMY_SKELETON) return texSkeleton;
    if(t == ENEMY_TROLL) return texTroll;
    if(t == ENEMY_CRATE) return texCrate;
    if(t == ENEMY_WEAPON) return texATT;
    if(t == ENEMY_VINES) return texVines;
    if(t == ENEMY_CHEST) return texChest;
    if(t == ENEMY_FOOD) return texST;
    if(t == ENEMY_POTION) return texPotion;
    return texChest;
}

void TryPlayerMove(int dx, int dy) {
    int nx = player.x + dx;
    int ny = player.y + dy;
    if(!Inside(nx, ny)) return;
    Enemy* e = EnemyAt(nx, ny);
    if(e) {
        int dmg = (player.atk > 0) ? GetRandomValue(1, player.atk) : 0;
        e->hp -= dmg;
        e->hitTimer = 0.4f;
        SpawnDamage(e->x, e->y, dmg, false);
        if(e->hp <= 0) {
            if(e->atk>0) player.fun += 1;
            player.treasure += e->treasure;
            if(e->food < 0) player.atk += -e->food;
            else player.stamina += e->food;
        }
        return;
    }
    if(EmptyTile(nx, ny)) {
        player.x = nx;
        player.y = ny;
    }
}


void PlayerAutoTurn() {
    // 1. Find nearest alive enemy
    Enemy* target = NULL;
    int bestDist = 9999;
    for(int i = 0; i < enemyCount; i++) {
        if(!enemies[i].alive || enemies[i].atk<=0) continue;
        int dist = abs(enemies[i].x - player.x) + abs(enemies[i].y - player.y);
        if(dist < bestDist){
            bestDist = dist;
            target = &enemies[i];
        }
    }
    if(bestDist>=2) bestDist *= 2; // prioritize close enough buffs (atk<=0)
    for(int i = 0; i < enemyCount; i++) {
        if(!enemies[i].alive || enemies[i].atk>0) continue;
        int dist = abs(enemies[i].x - player.x) + abs(enemies[i].y - player.y);
        if(dist < bestDist) {
            bestDist = dist;
            target = &enemies[i];
        }
    }

    if(!target) {
        // optional: random movement when no enemies
        int dx[4] = {1,-1,0,0};
        int dy[4] = {0,0,1,-1};
        int d = GetRandomValue(0,3);
        int nx = player.x + dx[d];
        int ny = player.y + dy[d];
        if(EmptyTile(nx,ny)) {
            player.x = nx;
            player.y = ny;
        }
        return;
    }

    int dx = target->x - player.x;
    int dy = target->y - player.y;
    if(abs(dx) + abs(dy) == 1) {
        int dmg = (player.atk > 0) ? GetRandomValue(1, player.atk) : 0;
        if(player.atk>1 && GetRandomValue(0,99)<10 && !interruptMessage) {
            player.atk--;
            interruptMessage = "LESS DMG";
        }
        target->hp -= dmg;
        target->hitTimer = 0.4f;
        SpawnDamage(target->x, target->y, dmg, false);
        if(target->hp <= 0)
        {
            if(target->atk>0) player.fun += 1;
            player.treasure += target->treasure;
            if(target->food<0) player.atk += -target->food;
            else player.stamina += target->food;
        }
        return;
    }

    // 3. Try to step toward target
    int stepX = 0;
    int stepY = 0;

    // Prefer axis with larger distance
    if(abs(dx) > abs(dy))
        stepX = (dx > 0) ? 1 : -1;
    else if(dy != 0)
        stepY = (dy > 0) ? 1 : -1;

    int nx = player.x + stepX;
    int ny = player.y + stepY;

    // If blocked, try alternate axis
    if(!EmptyTile(nx, ny))
    {
        stepX = 0;
        stepY = 0;
        if(abs(dx) <= abs(dy) && dx != 0)
            stepX = (dx > 0) ? 1 : -1;
        else if(dy != 0)
            stepY = (dy > 0) ? 1 : -1;
        nx = player.x + stepX;
        ny = player.y + stepY;
    }
    if(EmptyTile(nx, ny))
    {
        player.x = nx;
        player.y = ny;
    }
}


void EnemyTurn() {
    int has_been_damaged = 0;
    int dx[4] = {1, -1, 0, 0};
    int dy[4] = {0, 0, 1, -1};
    for(int i = 0; i < enemyCount; i++) {
        if(!enemies[i].alive)
            continue;
        if(enemies[i].hp<=0) {
            enemies[i].hp = 0;
            enemies[i].alive = 0;
        }
        if(abs(enemies[i].x - player.x) + abs(enemies[i].y - player.y) == 1 && enemies[i].atk) {
            int dmg = (enemies[i].atk > 0) ? GetRandomValue(1, enemies[i].atk) : 0;
            player.hp -= dmg;
            has_been_damaged += dmg;
            player.hitTimer = 0.4f;
            SpawnDamage(player.x, player.y, dmg, true);
            continue;
        }
        if(enemies[i].canMove) {
            int d = GetRandomValue(0, 3);
            int nx = enemies[i].x + dx[d];
            int ny = enemies[i].y + dy[d];
            if(EmptyTile(nx, ny)) {
                enemies[i].x = nx;
                enemies[i].y = ny;
            }
        }
    }
    if(!has_been_damaged && player.hp<20 && player.stamina) {
        player.hp += 1;
        player.stamina -= 1;
        SpawnDamage(player.x, player.y, -1, true);
    }
}

void PlaceEnemyFromHand(int slot) {
    if(enemyCount >= MAX_ENEMIES) return;
    if(hand[slot] == ENEMY_NONE) return;
    interruptMessage = nullptr;
    spaceCounter = 0;
    Enemy def = enemyTemplates[hand[slot]];
    int x = -1;
    int y = -1;
    if(def.hp == 1) {
        int dx[4] = { 1, -1, 0, 0 };
        int dy[4] = { 0,  0, 1, -1 };
        for(int i = 0; i < 4; i++) {
            int nx = player.x + dx[i];
            int ny = player.y + dy[i];

            if(EmptyTile(nx, ny)) {
                x = nx;
                y = ny;
                break;
            }
        }
        if(x == -1) {
            int attempts = 0;
            do {
                x = GetRandomValue(1, GRID - 2);
                y = GetRandomValue(1, GRID - 2);
                attempts++;
                if(attempts > 100) return;
            } while(!EmptyTile(x, y));
        }
    }
    else {
        int attempts = 0;
        do {
            x = GetRandomValue(1, GRID - 2);
            y = GetRandomValue(1, GRID - 2);
            attempts++;
            if(attempts > 100) return;
        } while(!EmptyTile(x, y));
    }
    enemies[enemyCount++] = MakeEnemy(hand[slot], x, y);
    for(int i = slot; i < HAND_SIZE - 1; i++)
        hand[i] = hand[i + 1];
    hand[HAND_SIZE - 1] = ENEMY_NONE;
    if(slot == 0) hand[0] = RandomEnemyType();
}

void DrawStat(Texture2D icon, const char *value, int x, int y) {
    int iconSize = 32;
    DrawScaled(icon, x, y, iconSize);
    DrawText(value, x + iconSize + 10, y + iconSize / 4, 24, WHITE);
}

void DrawGame() {
    // top bar
    DrawRectangle(0, 0, GRID * TILE, STATUS_BAR_SIZE, DARKGRAY);
    DrawStat(texHP, TextFormat("HP %d", player.hp), 16, 16);
    DrawStat(texATT, TextFormat("DMG %d", player.atk), 16+140, 16);
    DrawStat(texST, player.stamina ? TextFormat("Food %d", player.stamina) : "HUNGRY", 16+140*2, 16);
    if(luckcontrols) DrawStat(texTR, TextFormat("Gold %d", player.treasure), 16+140*3, 16);
    DrawStat(texMP, TextFormat("Sir K'S fun %d", player.fun), 16+140*4, 16);
    // terrain
    for (int y = 0; y < GRID; y++)
        for (int x = 0; x < GRID; x++)
            DrawTileBack(x, y);
    for (int i = 0; i < enemyCount; i++) {
        if (!enemies[i].alive) continue;
        Color tint = WHITE;
        if(enemies[i].hitTimer > 0.0f) {
            float duration = 0.4f;
            float elapsed  = duration - enemies[i].hitTimer;
            float pulse = fabsf(sinf(elapsed * 30.0f));
            float fade  = enemies[i].hitTimer / duration;
            pulse *= fade;
            unsigned char r = 255;
            unsigned char g = (unsigned char)(255 * (1.0f - pulse));
            unsigned char b = (unsigned char)(255 * (1.0f - pulse));
            tint = (Color){ r, g, b, 255 };
        }
        Texture2D tex = GetEnemyTexture(enemies[i].type);
        DrawTexturePro(
            tex,
            (Rectangle){0,0,(float)tex.width,(float)tex.height},
            (Rectangle){(float)(enemies[i].x * TILE), (float)(enemies[i].y * TILE + STATUS_BAR_SIZE), (float)TILE, (float)TILE},
            (Vector2){0,0}, 0, tint
        );
        int hp = enemies[i].hp;
        if(hp < 0) hp = 0;
        int pips = (hp + 1) / 2;
        int pipWidth  = 4;
        int pipHeight = 6;
        int spacing   = 2;
        int startX = enemies[i].x * TILE + (TILE - pipWidth*pips*1.5) / 2;
        int startY = enemies[i].y * TILE + STATUS_BAR_SIZE + (TILE - pipHeight*4) / 2 - 18;
        for(int p = 0; p < pips; p++)
            DrawRectangle(startX + p * (pipWidth + spacing), startY, pipWidth, pipHeight, (Color){120, 0, 0, 255});
    }
    //player
    Color playerTint = WHITE;
    if(player.hitTimer > 0.0f) {
        float duration = 0.4f;
        float elapsed  = duration - player.hitTimer;
        float pulse = fabsf(sinf(elapsed * 30.0f));
        float fade  = player.hitTimer / duration;
        pulse *= fade;
        unsigned char r = 255;
        unsigned char g = (unsigned char)(255 * (1.0f - pulse));
        unsigned char b = (unsigned char)(255 * (1.0f - pulse));
        playerTint = (Color){ r, g, b, 255 };
    }

    DrawTexturePro(
        texPlayer,
        (Rectangle){0,0,(float)texPlayer.width,(float)texPlayer.height},
        (Rectangle){(float)(player.x * TILE), (float)(player.y * TILE + STATUS_BAR_SIZE), (float)TILE, (float)TILE},
        (Vector2){0,0}, 0, playerTint
    );
    // damage
    for(int i = 0; i < MAX_DAMAGE_EVENTS; i++) {
        if(!damageEvents[i].active) continue;
        float progress = 1.0f - (damageEvents[i].timer / DAMAGE_LINGER_DURATION);
        float px = damageEvents[i].x * TILE + TILE/2;
        float py = damageEvents[i].y * TILE + STATUS_BAR_SIZE;
        py -= progress * 40.0f+20;
        float alpha = damageEvents[i].timer / DAMAGE_LINGER_DURATION;
        if(damageEvents[i].amount > 0)
            DrawText(TextFormat("%d", -damageEvents[i].amount), px - 12, py, 38, Fade((Color){120, 0, 0, 255}, alpha));
        else
            DrawText(TextFormat("+%d", damageEvents[i].amount), px - 12, py, 38, Fade((Color){0, 120, 0, 255}, alpha));
    }
    // terrain front
    for (int y = 0; y < GRID; y++)
        for (int x = 0; x < GRID; x++)
            DrawTileFront(x, y);
    // luck controls
    DrawRectangle(0, STATUS_BAR_SIZE + GRID * TILE, GRID * TILE, 2 * PREVIEW_SIZE + 80, DARKGRAY);
    if(!luckcontrols) {
        DrawText("WASD to move", 24, GRID * TILE + STATUS_BAR_SIZE + 10, 24, YELLOW);
        return;
    }
    for (int i = 0; i < HAND_SIZE; i++) {
        int baseX = 20 + i * (PREVIEW_SIZE + 20);
        int baseY = GRID * TILE + 90 + STATUS_BAR_SIZE;
        int padding = 5;
        DrawScaledDistorted(texCard, baseX, baseY+10, PREVIEW_SIZE, PREVIEW_SIZE*3/2);
        if(hand[i] == ENEMY_NONE) {
            DrawText("ENTER to buy", baseX + padding+4, baseY - 18, 24, YELLOW);
            const char* line1 = "";
            const char* line2 = "Costs";
            const char* line3 = TextFormat("%d", (i+1)/2);
            int centerX = baseX + PREVIEW_SIZE/2;
            int textY   = baseY + PREVIEW_SIZE/2;
            int w1 = MeasureText(line1, 22);
            int w2 = MeasureText(line2, 22);
            int w3 = MeasureText(line3, 18);
            DrawText(line1, centerX - w1/2, textY, 24, YELLOW);
            DrawText(line2, centerX - w2/2, textY + 22, 24, WHITE);
            int iconSize = 24;
            int totalWidth = iconSize + 6 + w3;
            int startX = centerX - totalWidth/2;
            DrawScaled(texTR, startX, textY + 46, iconSize);
            DrawText(line3, startX + iconSize + 6, textY + 46, 24, GOLD);
            break;
        }
        Enemy def = enemyTemplates[hand[i]];
        int portraitSize = PREVIEW_SIZE - padding*2;
        DrawScaled(GetEnemyTexture(hand[i]), baseX + padding, baseY + padding, portraitSize);
        DrawText(TextFormat("%d to spawn", i+1), baseX + padding-1, baseY - 18, 24, YELLOW);
        int statY = baseY + padding + portraitSize;
        baseX += 8;
        int iconSize = 24;
        statY += 6;
        if(def.atk > 0) {
            DrawScaled(texATT, baseX + PREVIEW_SIZE - padding - iconSize - 32-6, statY, iconSize);
            DrawText(TextFormat("%d", def.atk), baseX + PREVIEW_SIZE - padding - 32, statY, 24, WHITE);
        }
        else if(def.atk<0) {
            DrawScaled(texHP, baseX + PREVIEW_SIZE - padding - iconSize - 32-6, statY, iconSize);
            DrawText(TextFormat("%d", def.atk), baseX + PREVIEW_SIZE - padding - 32, statY, 24, WHITE);
        }
        if(def.hp==1) DrawText("Buff", baseX + padding, statY, 24, WHITE);
        else if(def.hp > 0) {
            DrawScaled(texHP, baseX + padding, statY, iconSize);
            DrawText(TextFormat("%d", def.hp), baseX + padding + iconSize + 4, statY, 24, WHITE);
        }
            statY += 32;
            
        if(def.treasure > 0) {
            DrawText("Win ", baseX + padding, statY, 24, GOLD);
            baseX += 50;
            DrawScaled(texTR, baseX + padding, statY, iconSize);
            DrawText(TextFormat("%d", def.treasure), baseX + padding + iconSize + 4, statY + 2, 24, GOLD);
            baseX -= 50;
        }
        if(def.food > 0) {
            DrawText("Win ", baseX + padding, statY, 24, GOLD);
            baseX += 50;
            DrawScaled(texST, baseX + padding, statY, iconSize);
            DrawText(TextFormat("%d", def.food), baseX + padding + iconSize + 4, statY + 2, 24, GOLD);
            baseX -= 50;
        }
        if(def.food < 0) {
            DrawText("Win ", baseX + padding, statY, 24, GOLD);
            baseX += 50;
            DrawScaled(texATT, baseX + padding, statY, iconSize);
            DrawText(TextFormat("%d", -def.food), baseX + padding + iconSize + 4, statY + 2, 24, GOLD);
            baseX -= 50;
        }
    }
    if(sirkcontrols)
        DrawText(spaceCounter>=MAX_PASS_COUNTER-1?"WASD or SPACE to automove":TextFormat("WASD or SPACE to automove: random spawn in %d turns", MAX_PASS_COUNTER-spaceCounter),
                24, GRID * TILE + STATUS_BAR_SIZE + 10, 24, YELLOW);
    else
        DrawText(spaceCounter>=MAX_PASS_COUNTER-1?"SPACE to automove":TextFormat("SPACE to automove: random spawn in %d turns", MAX_PASS_COUNTER-spaceCounter),
                24, GRID * TILE + STATUS_BAR_SIZE + 10, 24, YELLOW);
}

void HandlePressureSpawn() {
    if(spaceCounter==MAX_PASS_COUNTER-1 && luckcontrols) interruptMessage = "Random spawn next";
    if(spaceCounter < MAX_PASS_COUNTER) return;
    int x, y;
    int attempts = 0;
    do {
        x = GetRandomValue(1, GRID - 2);
        y = GetRandomValue(1, GRID - 2);
        attempts++;
        if(attempts > 100) break;
    }
    while(!EmptyTile(x, y));
    if(attempts <= 100 && enemyCount < MAX_ENEMIES) {
        for(int tries = 0; tries < 1000; tries++) {
            EnemyType roll = static_cast<EnemyType>(GetRandomValue(0, ENEMY_NONE - 1));
            bool inHand = false;
            for(int h = 0; h < HAND_SIZE; h++)
                if(hand[h] == roll) {
                    inHand = true;
                    break;
                }
            if(!inHand) {
                enemies[enemyCount] = MakeEnemy(roll, x, y);
                if(!luckcontrols) break; // if we don't control luck, pretend that it's uniformly random
                if(GetRandomValue(0,99)<75 && enemies[enemyCount].atk<=0) {}
                else break;
            }
        }
        enemyCount++;
    }
    spaceCounter = 0;
}

void ResetGame() {
    enemyCount = 0;
    spaceCounter = 0;
    interruptMessage = nullptr;
    InitGrid();
    player = (Player){GRID / 2, GRID / 2, 20, 4, 10, 5, 0};
    player.hitTimer = 0.0f;
    player.fun = 0;
    for(int i = 0; i < MAX_DAMAGE_EVENTS; i++)
        damageEvents[i].active = false;
    for (int i = 0; i < HAND_SIZE; i++)
        hand[i] = ENEMY_NONE;
    if(luckcontrols)
        for (int i = 0; i < HAND_SIZE; i++)
            hand[i] = RandomEnemyType();
}

int main() {
    InitWindow(GRID * TILE, GRID * TILE + 2 * PREVIEW_SIZE + STATUS_BAR_SIZE + 80, "Sir K's Funtime");
    SetTargetFPS(60);
    srand(time(NULL));
    gameFont = LoadFontEx("fonts/Beholden-Regular.ttf", 24, 0, 0);
    SetTextureFilter(gameFont.texture, TEXTURE_FILTER_BILINEAR);
    texCard = LoadTexture("images/card.png");
    texPlayer = LoadTexture("images/player.png");
    texVampire = LoadTexture("images/vampire.png");
    texSkeleton = LoadTexture("images/skeleton.png");
    texTroll = LoadTexture("images/troll.png");
    texPotion = LoadTexture("images/potion.png");
    texCrate = LoadTexture("images/crate.png");
    texVines = LoadTexture("images/vines.png");
    texChest = LoadTexture("images/chest.png");
    texWall = LoadTexture("images/wall.png");
    texFloor = LoadTexture("images/floor.png");
    texPillar = LoadTexture("images/pillar.png");
    texHP = LoadTexture("images/hp.png");
    texST = LoadTexture("images/stamina.png");
    texMP = LoadTexture("images/mana.png");
    texTR = LoadTexture("images/treasure.png");
    texATT = LoadTexture("images/attack.png");
    InitGrid();
    player = (Player){GRID / 2, GRID / 2, 20, 4, 10, 5, 0};
    player.hitTimer = 0.0f;

    for (int i = 0; i < HAND_SIZE; i++)
        hand[i] = ENEMY_NONE;
    for (int i = 0; i < HAND_SIZE/2; i++)
        hand[i] = RandomEnemyType();
    while (!WindowShouldClose())
    {
        if(gameState == GAME_SPLASH) {
            if(IsKeyPressed(KEY_SPACE))
            {
                ResetGame();
                gameState = GAME_PLAYING;
            }
            if(IsKeyPressed(KEY_A) || IsKeyPressed(KEY_D)) {
                int mode = 2; // both
                if(sirkcontrols && !luckcontrols) mode = 0;
                else if(!sirkcontrols && luckcontrols) mode = 1;
                if(IsKeyPressed(KEY_A)) mode = (mode + 2) % 3;
                else mode = (mode + 1) % 3;
                if(mode == 0) { sirkcontrols = 1; luckcontrols = 0; }
                if(mode == 1) { sirkcontrols = 0; luckcontrols = 1; }
                if(mode == 2) { sirkcontrols = 1; luckcontrols = 1; }
            }

        }
        else if (player.hp <= 0) {
            if(IsKeyPressed(KEY_SPACE))
                gameState = GAME_SPLASH;
        }
        else 
        {   
            if (sirkcontrols && IsKeyPressed(KEY_W)) {
                interruptMessage = nullptr;
                TryPlayerMove(0, -1);
                EnemyTurn();
                spaceCounter++;
                HandlePressureSpawn();
            }
            else if (sirkcontrols && IsKeyPressed(KEY_S)) {
                interruptMessage = nullptr;
                TryPlayerMove(0, 1);
                EnemyTurn();
                spaceCounter++;
                HandlePressureSpawn();
            }
            else if (sirkcontrols && IsKeyPressed(KEY_A)) {
                interruptMessage = nullptr;
                TryPlayerMove(-1, 0);
                EnemyTurn();
                spaceCounter++;
                HandlePressureSpawn();
            }
            else if (sirkcontrols && IsKeyPressed(KEY_D)) {
                interruptMessage = nullptr;
                TryPlayerMove(1, 0);
                EnemyTurn();
                spaceCounter++;
                HandlePressureSpawn();
            }
            else if (luckcontrols && IsKeyPressed(KEY_SPACE)) {
                interruptMessage = nullptr;
                PlayerAutoTurn();
                EnemyTurn();
                spaceCounter++;
                HandlePressureSpawn();
            }
            else if (luckcontrols && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))) {
                for (int i = 0; i < HAND_SIZE; i++) {
                    if (hand[i] == ENEMY_NONE) {
                        int cost = (i+1)/1;
                        if (player.treasure >= cost) {
                            player.treasure -= cost;
                            hand[i] = RandomEnemyType();
                        }
                        break;
                    }
                }
            }
            else if (luckcontrols) 
                for (int i = 0; i < HAND_SIZE; i++)
                    if (IsKeyPressed(KEY_ONE + i) || IsKeyPressed(KEY_KP_1 + i)) 
                        PlaceEnemyFromHand(i);
        }

        float dt = GetFrameTime();
        player.hitTimer -= dt;
        if(player.hitTimer < 0) player.hitTimer = 0;

        for(int i = 0; i < enemyCount; i++) {
            enemies[i].hitTimer -= dt;
            if(enemies[i].hitTimer < 0) enemies[i].hitTimer = 0;
        }

        for(int i = 0; i < MAX_DAMAGE_EVENTS; i++) {
            if(!damageEvents[i].active) continue;
            damageEvents[i].timer -= dt;
            if(damageEvents[i].timer <= 0)
                damageEvents[i].active = false;
        }

        BeginDrawing();
        ClearBackground(BLACK);
        if(gameState == GAME_SPLASH)
        {
            int screenW = GetScreenWidth();
            int screenH = GetScreenHeight();
            DrawRectangle(0, 0, screenW, screenH, BLACK);
            int titleSize = 80;
            int promptSize = 32;
            DrawText("SIR K'S FUNTIME", screenW/2 - MeasureText("SIR K'S FUNTIME", titleSize)/2, screenH/2 - 200, titleSize, WHITE);
            DrawText("AD to change controls / SPACE to start", screenW/2 - MeasureText("AD to change controls / SPACE to start", promptSize)/2, screenH/2 + 160, promptSize, Fade(YELLOW, 0.8));
            const char* opt1 = "Sir K";
            const char* opt2 = "Spawns";
            const char* opt3 = "Both";
            float pulse = 0.5f + 0.5f * sinf(GetTime() * 4.0f);
            Color c1 = (sirkcontrols && !luckcontrols) ? Fade(WHITE, 0.6f + 0.4f*pulse) : Fade(GRAY, 0.7f);
            Color c2 = (!sirkcontrols && luckcontrols) ? Fade(WHITE, 0.6f + 0.4f*pulse) : Fade(GRAY, 0.7f);
            Color c3 = (sirkcontrols && luckcontrols) ? Fade(WHITE, 0.6f + 0.4f*pulse) : Fade(GRAY, 0.7f);
            DrawText(opt1, screenW/2 - MeasureText(opt1, promptSize)/2-100, screenH/2 + 210, promptSize, c1);
            DrawText(opt2, screenW/2 - MeasureText(opt2, promptSize)/2, screenH/2 + 210, promptSize, c2);
            DrawText(opt3, screenW/2 - MeasureText(opt3, promptSize)/2+100, screenH/2 + 210, promptSize, c3);
            int iconSize = 96;
            int spacing = 0;
            Texture2D icons[4] = { texPlayer, texATT, texTroll };
            int totalWidth = iconSize * 3 + spacing *2;
            int startX = screenW/2 - totalWidth/2;
            int y = screenH/2 - 40;
            for(int i = 0; i < 3; i++){
                DrawTexturePro(
                    icons[i],
                    (Rectangle){0,0,(float)icons[i].width,(float)icons[i].height},
                    (Rectangle){(float)(startX + i*(iconSize + spacing)),(float)y,(float)iconSize,(float)iconSize},
                    (Vector2){0,0}, 0, WHITE
                );
            }
            EndDrawing();
            continue;
        }
        DrawGame();
        if (player.hp <= 0) {
            int screenW = GetScreenWidth();
            int screenH = GetScreenHeight();
            DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.8f));
            const char *msg = TextFormat("SIR K'S FUN %d", player.fun);
            int fontSize = 64;
            int textW = MeasureText(msg, fontSize);
            int textH = fontSize;
            int panelPadding = 40;
            int panelW = textW + panelPadding * 2;
            int panelH = textH + panelPadding * 2;
            int panelX = (screenW - panelW) / 2;
            int panelY = (screenH - panelH) / 2-150;
            DrawRectangleRounded((Rectangle){(float)panelX, (float)panelY, (float)panelW, (float)panelH}, 0.2f, 12, DARKGRAY);
            DrawRectangleRoundedLines((Rectangle){(float)panelX, (float)panelY, (float)panelW, (float)panelH}, 0.2f, 12, RED);
            DrawText(msg, panelX + (panelW - textW) / 2, panelY + (panelH - textH) / 2, fontSize, RED);
        }
        else if(interruptMessage) {
            int screenW = GetScreenWidth();
            int screenH = GetScreenHeight();
            DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.3f));
            int fontSize = 64;
            int textW = MeasureText(interruptMessage, fontSize);
            int textH = fontSize;
            int panelPadding = 40;
            int panelW = textW + panelPadding * 2;
            int panelH = textH + panelPadding * 2;
            int panelX = (screenW - panelW) / 2;
            int panelY = (screenH - panelH) / 2;
            DrawText(interruptMessage, panelX + (panelW - textW) / 2, panelY + (panelH - textH) / 2-150, fontSize, YELLOW);
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
