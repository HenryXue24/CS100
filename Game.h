#pragma once

/// \file
/// \brief This file contains the game lifecycle and logics.
/// There are 5 important functions:
/// `GameInit`, `GameInput`, `GameUpdate`, `GameTerminate`, and
/// the most important: `GameLifecycle`.
/// Please read the corresponding comments to understand their relationships.

//
//
//
//
//
#include "Config.h"
#include "Renderer.h"

#include <time.h>

typedef struct {
  char keyHit; // The keyboard key hit by the player at this frame.
} Game;

// The game singleton.
static Game game;

// The keyboard key "ESC".
static const char keyESC = '\033';

//
//
//
/// \brief Configure the scene (See `Scene.h`) with `config` (See `Config.h`), and
/// perform initializations including:
/// 1. Terminal setup.
/// 2. Memory allocations.
/// 3. Map and object generations.
/// 4. Rendering of the initialized scene.
///
/// \note This function should be called at the very beginning of `GameLifecycle`.
//
// 随机生成 'a', 'w', 's', 'd' 字符的函数
char RandomAWSD(void) {
  int randomValue = rand() % 4; // 生成 0 到 3 的随机数

  switch (randomValue) {
  case 0:
    return 'a'; // Left
  case 1:
    return 'w'; // Up
  case 2:
    return 's'; // Down
  case 3:
    return 'd'; // Right
  default:
    return '\0'; // 不应该发生
  }
}

char RandomDirection(void) {
  int randomValue = rand() % 4; // 生成 0 到 3 的随机数

  switch (randomValue) {
  case 0:
    return eDirOP; // Up
  case 1:
    return eDirNO; // Left
  case 2:
    return eDirON; // Down
  case 3:
    return eDirPO; // Right
  default:
    return '\0'; // 不应该发生
  }
}

bool is_pos_valid(Vec pos, Tank *currentTank) {
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      Vec temp_pos = Add(pos, (Vec){dx, dy});
      if (temp_pos.x <= 0 || temp_pos.x >= map.size.x - 1 || temp_pos.y <= 0 || temp_pos.y >= map.size.y - 1)
        return false;
      if (map.flags[Idx(temp_pos)] == eFlagSolid || map.flags[Idx(temp_pos)] == eFlagWall)
        return false;
    }
  }
  // 检查与其他坦克的重叠
  for (RegIterator it = RegBegin(regTank); it != RegEnd(regTank); it = RegNext(it)) {
    Tank *temp_tank = RegEntry(regTank, it);
    if (temp_tank == currentTank && currentTank != NULL) // 忽略当前坦克
      continue;
    Vec original_pos = temp_tank->pos;
    if (abs(original_pos.x - pos.x) < 3 && abs(original_pos.y - pos.y) < 3)
      return false;
  }
  // 如果所有检查都通过，返回 true
  return true;
}

// 随机生成可行的tank坐标
void Generate_Position(Tank *tank) {
  Vec validPositions[1000]; // 假设最多有 1000 个有效位置
  int validCount = 0;
  // 遍历地图，找到所有有效位置
  for (int y = 2; y < map.size.y - 3; y++) {
    for (int x = 2; x < map.size.x - 3; x++) {
      Vec pos = {x, y};
      if (is_pos_valid(pos, tank))
        validPositions[validCount++] = pos;
    }
  }
  // 随机选择一个有效位置
  if (validCount > 0) {
    tank->pos = validPositions[ramdom_generator(0, validCount - 1)];
  } else {
    exit(1); // 或者采取其他错误处理方式
  }
}

void GameInit(void) {
  // Setup terminal.
  TermSetupGameEnvironment();
  TermClearScreen();

  // Configure scene.
  map.size = config.mapSize;
  int nEnemies = config.nEnemies;
  int nSolids = config.nSolids;
  int nWalls = config.nWalls;

  // Initialize scene.在真正使用前都必须要调用gefinit()初始化
  RegInit(regTank);
  RegInit(regBullet);
  // 初始化边框
  map.flags = (Flag *)malloc(sizeof(Flag) * map.size.x * map.size.y);
  for (int y = 0; y < map.size.y; ++y)
    for (int x = 0; x < map.size.x; ++x) {
      Vec pos = {x, y};

      Flag flag = eFlagNone;
      if (x == 0 || y == 0 || x == map.size.x - 1 || y == map.size.y - 1)
        flag = eFlagSolid;

      map.flags[Idx(pos)] = flag;
    }
  // 初始化3*3的#wall
  for (int i = 0; i < nWalls; i++) {
    Vec pos = RandPos();
    if (is_pos_valid(pos, NULL)) {
      // 遍历3x3区域
      for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
          // 计算当前位置
          Vec currentPos = {pos.x + x, pos.y + y};
          // 绘制 # 字符
          map.flags[Idx(currentPos)] = eFlagWall;
        }
      }
    }
  }
  // 初始化3*3的%solid
  for (int i = 0; i < nSolids; i++) {
    Vec pos = RandPos();
    if (is_pos_valid(pos, NULL)) {
      // 遍历3x3区域
      for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
          // 计算当前位置
          Vec currentPos = {pos.x + x, pos.y + y};
          // 绘制 % 字符
          map.flags[Idx(currentPos)] = eFlagSolid;
        }
      }
    }
  }
  // 初始化坦克
  {
    Tank *tank = RegNew(regTank);
    tank->dir = RandomDirection();
    tank->color = TK_GREEN;
    tank->isPlayer = true;
    Generate_Position(tank);
    tank->flag_check = true;
  }
  for (int i = 0; i < nEnemies - 1; i++) {
    {
      Tank *tank = RegNew(regTank);
      tank->dir = RandomDirection();
      tank->color = TK_RED;
      tank->isPlayer = false;
      tank->flag_check = true;
      Generate_Position(tank);
    }
  }

  // Initialize renderer.
  renderer.csPrev = (char *)malloc(sizeof(char) * map.size.x * map.size.y);
  renderer.colorsPrev = (Color *)malloc(sizeof(Color) * map.size.x * map.size.y);
  renderer.cs = (char *)malloc(sizeof(char) * map.size.x * map.size.y);
  renderer.colors = (Color *)malloc(sizeof(Color) * map.size.x * map.size.y);

  for (int i = 0; i < map.size.x * map.size.y; ++i) {
    renderer.csPrev[i] = renderer.cs[i] = ' ';
    renderer.colorsPrev[i] = renderer.colors[i] = TK_NORMAL;
  }

  // Render scene.
  for (int y = 0; y < map.size.y; ++y)
    for (int x = 0; x < map.size.x; ++x) {
      Vec pos = {x, y};
      RdrPutChar(pos, map.flags[Idx(pos)], TK_AUTO_COLOR);
    }
  RdrRender();
  RdrFlush();
}

//
//
//
/// \brief Read input from the player.
///
/// \note This function should be called in the loop of `GameLifecycle` before `GameUpdate`.
void GameInput(void) {
  game.keyHit = kbhit_t() ? (char)getch_t() : '\0';
}

//
//
//
/// \brief Perform all tasks required for a frame update, including:
/// 1. Game logics of `Tank`s, `Bullet`s, etc.
/// 2. Rerendering all objects in the scene.
///
/// \note This function should be called in the loop of `GameLifecycle` after `GameInput`.
void GameUpdate(void) {
  RdrClear();
  static int enemyMoveCounter = 0, enemyshootcount = 0, bullet_move = 0;
  // 控制敌方坦克移动频率
  enemyMoveCounter++;
  enemyshootcount++;
  bullet_move++;
  bool shouldMoveEnemies = (enemyMoveCounter >= 15);
  bool shouldshoot = (enemyshootcount >= 20);
  bool should_bullet_move = (bullet_move >= 20);
  if (shouldMoveEnemies)
    enemyMoveCounter = 0; // 重置计数器
  if (shouldshoot)
    enemyshootcount = 0;
  // 控制tank的移动
  for (RegIterator it = RegBegin(regTank); it != RegEnd(regTank); it = RegNext(it)) {
    Tank *tank = RegEntry(regTank, it);
    tank->flag_check = false;
    char temp;
    if (tank->isPlayer) {
      temp = game.keyHit; // 玩家输入
    } else {
      if (shouldMoveEnemies) {
        temp = RandomAWSD();
      } else {
        continue; // 如果不该移动，跳过本次更新
      }
    }
    Vec targetPos = tank->pos;
    switch (temp) {
    case 'w': // Up
      if (tank->dir == eDirOP)
        targetPos.y += 1;
      else
        tank->dir = eDirOP;
      break;
    case 's': // Down
      if (tank->dir == eDirON)
        targetPos.y -= 1;
      else
        tank->dir = eDirON;
      break;
    case 'a': // Left
      if (tank->dir == eDirNO)
        targetPos.x -= 1;
      else
        tank->dir = eDirNO;
      break;
    case 'd': // Right
      if (tank->dir == eDirPO)
        targetPos.x += 1;
      else
        tank->dir = eDirPO;
      break;
    }
    if (is_pos_valid(targetPos, tank))
      tank->pos = targetPos;
    tank->flag_check = true;
  }
  // // 控制子弹的生成
  // for (RegIterator it = RegBegin(regTank); it != RegEnd(regTank); it = RegNext(it)) {
  //   Tank *tank = RegEntry(regTank, it);
  //   Vec pos;
  //   // 玩家坦克发射子弹
  //   if (tank->isPlayer && game.keyHit == 'k') {
  //     switch (tank->dir) {
  //     case 0: // Down
  //       pos = (Vec){tank->pos.x, tank->pos.y - 1};
  //       break;
  //     case 1: // Left
  //       pos = (Vec){tank->pos.x - 1, tank->pos.y};
  //       break;
  //     case 2: // Right
  //       pos = (Vec){tank->pos.x + 1, tank->pos.y};
  //       break;
  //     case 3: // Up
  //       pos = (Vec){tank->pos.x, tank->pos.y + 1};
  //       break;
  //     }
  //     // 创建子弹
  //     Bullet *bullet = RegNew(regBullet);
  //     bullet->pos = pos;
  //     bullet->dir = tank->dir;
  //     bullet->isPlayer = true;
  //     bullet->color = TK_GREEN; // 玩家子弹颜色
  //   }
  //   // 敌方坦克发射子弹
  //   if (!tank->isPlayer && shouldshoot) {
  //     switch (tank->dir) {
  //     case 0: // Down
  //       pos = (Vec){tank->pos.x, tank->pos.y - 1};
  //       break;
  //     case 1: // Left
  //       pos = (Vec){tank->pos.x - 1, tank->pos.y};
  //       break;
  //     case 2: // Right
  //       pos = (Vec){tank->pos.x + 1, tank->pos.y};
  //       break;
  //     case 3: // Up
  //       pos = (Vec){tank->pos.x, tank->pos.y + 1};
  //       break;
  //     }
  //     // 创建子弹
  //     Bullet *bullet = RegNew(regBullet);
  //     bullet->pos = pos;
  //     bullet->dir = tank->dir;
  //     bullet->isPlayer = false;
  //     bullet->color = TK_RED; // 敌方子弹颜色
  //   }
  //   // 绘制子弹运动
  //   if (should_bullet_move) {
  //     for (RegIterator it = RegBegin(regBullet); it != RegEnd(regBullet); it = RegNext(it)) {
  //       Bullet *bullet = RegEntry(regBullet, it);
  //       Vec temp_pos;
  //       switch (bullet->dir) {
  //       case eDirON:
  //         temp_pos = (Vec){bullet->pos.x, bullet->pos.y + 1};
  //         break;
  //       case eDirNO:
  //         temp_pos = (Vec){bullet->pos.x - 1, bullet->pos.y};
  //         break;
  //       case eDirPO:
  //         temp_pos = (Vec){bullet->pos.x + 1, bullet->pos.y};
  //         break;
  //       case eDirOP:
  //         temp_pos = (Vec){bullet->pos.x, bullet->pos.y - 1};
  //         break;
  //       }
  //       bullet->pos = temp_pos;
  //     }
  //   }
  // }
  RdrRender();
  RdrFlush();
}

//
//
//
/// \brief Terminate the game and free all the resources.
///
/// \note This function should be called at the very end of `GameLifecycle`.
void GameTerminate(void) {
  while (RegSize(regTank) > 0)
    RegDelete(RegEntry(regTank, RegBegin(regTank)));

  while (RegSize(regBullet) > 0)
    RegDelete(RegEntry(regBullet, RegBegin(regBullet)));

  free(map.flags);

  free(renderer.csPrev);
  free(renderer.colorsPrev);
  free(renderer.cs);
  free(renderer.colors);

  TermClearScreen();
}

//
//
//
/// \brief Lifecycle of the game, defined by calling the 4 important functions:
/// `GameInit`, `GameInput`, `GameUpdate`, and `GameTerminate`.
///
/// \note This function should be called by `main`.
void GameLifecycle(void) {
  GameInit();

  double frameTime = (double)1000 / (double)config.fps;
  clock_t frameBegin = clock();

  while (true) {
    GameInput();
    if (game.keyHit == keyESC)
      break;

    GameUpdate();

    while (((double)(clock() - frameBegin) / CLOCKS_PER_SEC) * 1000.0 < frameTime - 0.5)
      Daze();
    frameBegin = clock();
  }

  GameTerminate();
}
