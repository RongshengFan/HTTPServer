#include "AiGame.h"

#include <chrono>
#include <thread>
#include <bits/stdc++.h>

AiGame::AiGame(int userId)
    : gameOver_(false)
    , userId_(userId)
    , moveCount_(0)
    , lastMove_(-1, -1)
    , board_(BOARD_SIZE, std::vector<std::string>(BOARD_SIZE, EMPTY))
{
	srand(time(0)); // 初始化随机数种子
}

// 处理人类玩家移动
bool AiGame::humanMove(int x, int y) 
{
    if (!isValidMove(x, y)) 
        return false;
    
    board_[x][y] = HUMAN_PLAYER;
    moveCount_++;
    lastMove_ = {x, y};
    
    if (checkWin(x, y, HUMAN_PLAYER)) 
    {
        gameOver_ = true;
        winner_ = "human";
    }
    return true;
}

 // AI移动
void AiGame::aiMove() 
{
    if (gameOver_ || isDraw()) return;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 添加500毫秒延时
    int x, y;
    // 获取AI的最佳移动位置
    std::tie(x, y) = getBestMove();
    board_[x][y] = AI_PLAYER;
    moveCount_++;
    lastMove_ = {x, y};
    
    if (checkWin(x, y, AI_PLAYER)) 
    {
        gameOver_ = true;
        winner_ = "ai";
    }
}


// 辅助函数：评估某个位置的威胁程度
int AiGame::evaluateThreat(int r, int c) 
{
    int threat = 0;

    // 检查四个方向上的玩家连子数
    const int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    for (auto& dir : directions) 
    {
        int count = 1;
        for (int i = 1; i <= 2; i++) 
        { // 探查2步
            int nr = r + i * dir[0], nc = c + i * dir[1];
            if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board_[nr][nc] == HUMAN_PLAYER) 
            {
                count++;
            }
        }
        threat += count; // 威胁分数累加
    }
    return threat;
}

// 辅助函数：判断某个空位是否靠近已有棋子
bool AiGame::isNearOccupied(int r, int c) 
{
    const int directions[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}
    };
    for (auto& dir : directions) 
    {
        int nr = r + dir[0], nc = c + dir[1];
        if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board_[nr][nc] != EMPTY) 
        {
            return true; // 该空位靠近已有棋子
        }
    }
    return false;
}

// 检查胜利条件
bool AiGame::checkWin(int x, int y, const std::string& player) 
{
    // 检查方向数组：水平、垂直、对角线、反对角线
    const int dx[] = {1, 0, 1, 1};
    const int dy[] = {0, 1, 1, -1};
    
    for (int dir = 0; dir < 4; dir++) 
    {
        int count = 1;  // 当前位置已经有一个棋子
        
        // 正向检查
        for (int i = 1; i < 5; i++) 
        {
            int newX = x + dx[dir] * i;
            int newY = y + dy[dir] * i;
            if (!isInBoard(newX, newY) || board_[newX][newY] != player) break;
            count++;
        }
        
        // 反向检查
        for (int i = 1; i < 5; i++) 
        {
            int newX = x - dx[dir] * i;
            int newY = y - dy[dir] * i;
            if (!isInBoard(newX, newY) || board_[newX][newY] != player) break;
            count++;
        }
        
        if (count >= 5) return true;
    }
    return false;
}

// std::pair<int, int> AiGame::getBestMove()
// {
//     std::pair<int, int> bestMove = {-1, -1}; // 最佳落子位置
//     int maxThreat = -1;                      // 记录最大的威胁分数
//
//     // 1. 优先尝试进攻获胜或阻止玩家获胜
//     for (int r = 0; r < BOARD_SIZE; r++) 
//     {
//         for (int c = 0; c < BOARD_SIZE; c++) 
//         {
//             if (board_[r][c] != EMPTY) continue; // 确保当前位置为空闲
//
//             // 模拟 AI 落子，判断是否可以获胜
//             board_[r][c] = AI_PLAYER;
//             if (checkWin(r, c, AI_PLAYER)) 
//             {
//                 // board_[r][c] = AI_PLAYER; // 恢复棋盘
//                 return {r, c};      // 立即获胜
//             }
//             board_[r][c] = EMPTY;
//
//             // 模拟玩家落子，判断是否需要防守
//             board_[r][c] = HUMAN_PLAYER;
//             if (checkWin(r, c, HUMAN_PLAYER)) 
//             {
//                 board_[r][c] = AI_PLAYER; // 恢复棋盘
//                 return {r, c};      // 立即防守
//             }
//             board_[r][c] = EMPTY;
//         }
//     }
//
//     // 2. 评估每个空位的威胁程度，选择最佳防守位置
//     for (int r = 0; r < BOARD_SIZE; r++) 
//     {
//         for (int c = 0; c < BOARD_SIZE; c++) 
//         {
//             if (board_[r][c] != EMPTY) continue; // 确保当前位置为空闲
//
//             int threatLevel = evaluateThreat(r, c); // 评估威胁程度
//             if (threatLevel > maxThreat) 
//             {
//                 maxThreat = threatLevel;
//                 bestMove = {r, c};
//             }
//         }
//     }
//
//     // 3. 如果找不到威胁点，选择靠近玩家或已有棋子的空位
//     if (bestMove.first == -1) 
//     {
//         std::vector<std::pair<int, int>> nearCells;
//
//         for (int r = 0; r < BOARD_SIZE; r++) 
//         {
//             for (int c = 0; c < BOARD_SIZE; c++) 
//             {
//                 if (board_[r][c] == EMPTY && isNearOccupied(r, c)) 
//                 { // 确保当前位置为空闲且靠近已有棋子
//                     nearCells.push_back({r, c});
//                 }
//             }
//         }
//
//         // 如果找到靠近已有棋子的空位，随机选择一个
//         if (!nearCells.empty()) 
// 		{
//             int num = rand();
// 			board_[nearCells[num % nearCells.size()].first][nearCells[num % nearCells.size()].second] = AI_PLAYER;
//             return nearCells[num % nearCells.size()];
//         }
//
//         // 4. 如果所有策略都无效，选择第一个空位（保证 AI 落子）
//         for (int r = 0; r < BOARD_SIZE; r++) 
//         {
//             for (int c = 0; c < BOARD_SIZE; c++) 
//             {
//                 if (board_[r][c] == EMPTY) 
// 				{
// 					board_[r][c] = AI_PLAYER;
//                     return {r, c}; // 返回第一个空位
//                 }
//             }
//         }
//     }
//
// 	board_[bestMove.first][bestMove.second] = AI_PLAYER;
//     return bestMove; // 返回最佳防守点或其他策略的结果
// }
//

std::pair<int, int> AiGame::getBestMove() 
{
    // 1. 先检查一步胜负（优先处理，避免无效搜索）
    for (int r = 0; r < BOARD_SIZE; r++) 
    {
        for (int c = 0; c < BOARD_SIZE; c++) 
        {
            if (board_[r][c] != EMPTY) continue;

            // 必胜步
            board_[r][c] = AI_PLAYER;
            if (checkWin(r, c, AI_PLAYER)) 
            {
                board_[r][c] = EMPTY;
                return {r, c};
            }
            board_[r][c] = EMPTY;

            // 必防步
            board_[r][c] = HUMAN_PLAYER;
            if (checkWin(r, c, HUMAN_PLAYER)) 
            {
                board_[r][c] = EMPTY;
                return {r, c};
            }
            board_[r][c] = EMPTY;
        }
    }

    // 2. 迭代加深搜索（逐步增加深度，找到足够好的解就退出）
    std::vector<std::pair<int, int>> candidates = getCandidateMoves();
    if (candidates.empty()) 
        return {BOARD_SIZE/2, BOARD_SIZE/2};  // 开局中心

    int bestScore = -1e9;
    std::pair<int, int> bestMove = candidates[0];
    int maxDepth = (moveCount_ < 10) ? 3 : (moveCount_ < 20) ? 4 : 5;  // 最大深度限制

    // 迭代加深：从浅到深搜索，找到高分解提前终止
    for (int depth = 2; depth <= maxDepth; depth++) 
    {
        int currentBestScore = -1e9;
        std::pair<int, int> currentBestMove = bestMove;

        for (auto& move : candidates) 
        {
            int x = move.first, y = move.second;
            if (board_[x][y] != EMPTY) continue;

            board_[x][y] = AI_PLAYER;
            lastMove_ = {x, y};
            int score = minimax(depth - 1, false, -1e9, 1e9, currentBestScore);
            board_[x][y] = EMPTY;

            if (score > currentBestScore) 
            {
                currentBestScore = score;
                currentBestMove = move;
                // 找到必胜解，直接返回（无需继续加深）
                if (currentBestScore >= 100000) 
                {
                    return currentBestMove;
                }
            }
        }

        // 更新最佳解
        bestScore = currentBestScore;
        bestMove = currentBestMove;

        // 如果当前分数已足够高，无需继续加深
        if (bestScore > 50000) break;
    }

    return bestMove;
}

// 优化：动态深度+剪枝强化
int AiGame::minimax(int depth, bool isMaximizing, int alpha, int beta, int& bestMoveScore) 
{
    // 1. 终端条件：深度为0或已出胜负
    if (depth == 0) 
    {
        return evaluateBoard();  // 全局评估（替代原双重循环评估）
    }

    // 2. 检查胜负（提前终止）
    if (checkWin(lastMove_.first, lastMove_.second, AI_PLAYER)) 
        return 100000 + depth;  // 深度加成：优先走近期能赢的棋
    if (checkWin(lastMove_.first, lastMove_.second, HUMAN_PLAYER)) 
        return -100000 - depth;
    if (isDraw()) 
        return 0;

    // 3. 动态调整深度（复杂局面加深，简单局面减浅）
    std::vector<std::pair<int, int>> candidates = getCandidateMoves();
    if (candidates.size() < 10 && depth < 5)  // 局面简单（候选点少），可加深
        depth++;
    else if (candidates.size() > 30 && depth > 2)  // 局面复杂，减浅
        depth--;

    // 4. 排序候选点（让高分点先被评估，加速剪枝）
    sortCandidates(candidates, isMaximizing);

    if (isMaximizing) 
    {
        int maxScore = -1e9;
        for (auto& move : candidates) 
        {
            int x = move.first, y = move.second;
            if (board_[x][y] != EMPTY) continue;

            board_[x][y] = AI_PLAYER;
            lastMove_ = {x, y};
            int score = minimax(depth - 1, false, alpha, beta, bestMoveScore);
            board_[x][y] = EMPTY;

            if (score > maxScore) 
            {
                maxScore = score;
                // 剪枝强化：如果已找到必胜解，直接返回
                if (maxScore >= 100000) return maxScore;
            }
            alpha = std::max(alpha, score);
            if (beta <= alpha) break;  // 剪枝
        }
        return maxScore;
    } 
    else 
    {
        int minScore = 1e9;
        for (auto& move : candidates) 
        {
            int x = move.first, y = move.second;
            if (board_[x][y] != EMPTY) continue;

            board_[x][y] = HUMAN_PLAYER;
            lastMove_ = {x, y};
            int score = minimax(depth - 1, true, alpha, beta, bestMoveScore);
            board_[x][y] = EMPTY;

            if (score < minScore) 
            {
                minScore = score;
                if (minScore <= -100000) return minScore;  // 提前返回
            }
            beta = std::min(beta, score);
            if (beta <= alpha) break;  // 剪枝
        }
        return minScore;
    }
}

// 辅助：对候选点排序（让高分点先评估，加速剪枝）
void AiGame::sortCandidates(std::vector<std::pair<int, int>>& candidates, bool isMaximizing) 
{
    // 根据当前玩家的威胁评分排序
    std::sort(candidates.begin(), candidates.end(), [&](const auto& a, const auto& b) {
        int scoreA = evaluateThreat(a.first, a.second, isMaximizing ? AI_PLAYER : HUMAN_PLAYER);
        int scoreB = evaluateThreat(b.first, b.second, isMaximizing ? AI_PLAYER : HUMAN_PLAYER);
        return scoreA > scoreB;  // 高分在前
    });
}

// 优化：全局评估函数（替代原双重循环，减少计算）
int AiGame::evaluateBoard() 
{
    int aiScore = 0, humanScore = 0;
    // 只评估候选位置（而非全棋盘）
    auto candidates = getCandidateMoves();
    for (auto& p : candidates) 
    {
        aiScore += evaluateThreat(p.first, p.second, AI_PLAYER);
        humanScore += evaluateThreat(p.first, p.second, HUMAN_PLAYER);
    }
    return aiScore - humanScore;
}

// 优化：评估函数减少循环次数，增加提前终止
int AiGame::evaluateThreat(int r, int c, const std::string& player) 
{
    if (board_[r][c] != EMPTY) return 0;
    
    board_[r][c] = player;
    int totalScore = 0;
    const int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    const std::string opponent = (player == AI_PLAYER) ? HUMAN_PLAYER : AI_PLAYER;
    
    for (auto& dir : directions) 
    {
        int lineScore = 0;
        int consecutive = 1;
        bool blockedLeft = false, blockedRight = false;

        // 合并左右检查（减少代码冗余）
        for (int side = -1; side <= 1; side += 2)  // 左(-1)和右(+1)
        {
            for (int i = 1; i <= 4; i++) 
            {
                int nr = r + side * i * dir[0];
                int nc = c + side * i * dir[1];
                if (!isInBoard(nr, nc) || board_[nr][nc] == opponent) 
                {
                    if (side == -1) blockedLeft = true;
                    else blockedRight = true;
                    break;
                }
                if (board_[nr][nc] == player) consecutive++;
                else break;  // 空位则停止延伸
            }
        }

        // 评分规则简化（减少分支判断）
        if (consecutive >= 5) lineScore = 100000;
        else if (consecutive == 4) lineScore = (!blockedLeft || !blockedRight) ? 10000 : 100;
        else if (consecutive == 3) 
        {
            int open = (!blockedLeft) + (!blockedRight);
            lineScore = (open == 2) ? 1000 : (open == 1 ? 100 : 10);
        }
        else if (consecutive == 2) 
        {
            int open = (!blockedLeft) + (!blockedRight);
            lineScore = (open == 2) ? 50 : (open == 1 ? 10 : 5);
        }
        else lineScore = ((!blockedLeft) + (!blockedRight) == 2) ? 5 : 1;

        totalScore += lineScore;
        // 提前终止：如果单方向已出高分，无需检查其他方向
        if (totalScore >= 10000) break;
    }
    
    board_[r][c] = EMPTY;
    return totalScore;
}

// 优化：更严格的候选位置筛选（减少50%+的搜索点）
std::vector<std::pair<int, int>> AiGame::getCandidateMoves() 
{
    std::vector<std::pair<int, int>> candidates;
    std::unordered_set<long long> uniqueMoves;  // 去重用（用哈希快速去重）

    // 只检查已有棋子周围2格内的位置（大幅减少候选点）
    for (int i = 0; i < BOARD_SIZE; i++) 
    {
        for (int j = 0; j < BOARD_SIZE; j++) 
        {
            if (board_[i][j] == EMPTY) continue;

            // 只搜索2x2范围内（原3x3缩小，进一步减少点）
            for (int dx = -2; dx <= 2; dx++) 
            {
                for (int dy = -2; dy <= 2; dy++) 
                {
                    if (abs(dx) + abs(dy) > 3) continue;  // 过滤对角线过远的点
                    int x = i + dx;
                    int y = j + dy;
                    if (isInBoard(x, y) && board_[x][y] == EMPTY) 
                    {
                        // 用哈希去重（比sort+unique高效）
                        long long key = (long long)x * BOARD_SIZE + y;
                        if (uniqueMoves.find(key) == uniqueMoves.end()) 
                        {
                            uniqueMoves.insert(key);
                            candidates.emplace_back(x, y);
                        }
                    }
                }
            }
        }
    }

    // 开局特殊处理（只保留中心及附近）
    if (moveCount_ < 5) 
    {
        std::vector<std::pair<int, int>> earlyGame;
        int center = BOARD_SIZE / 2;
        for (auto& p : candidates) 
        {
            int dx = abs(p.first - center);
            int dy = abs(p.second - center);
            if (dx <= 2 && dy <= 2)  // 中心3x3范围
                earlyGame.push_back(p);
        }
        return earlyGame.empty() ? candidates : earlyGame;
    }

    return candidates;
}