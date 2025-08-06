#include "AiGame.h"

#include <chrono>
#include <thread>


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


std::pair<int, int> AiGame::getBestMove()
{
    std::pair<int, int> bestMove = {-1, -1}; // 最佳落子位置
    int maxThreat = -1;                      // 记录最大的威胁分数

    // 1. 优先尝试进攻获胜或阻止玩家获胜
    for (int r = 0; r < BOARD_SIZE; r++) 
    {
        for (int c = 0; c < BOARD_SIZE; c++) 
        {
            if (board_[r][c] != EMPTY) continue; // 确保当前位置为空闲

            // 模拟 AI 落子，判断是否可以获胜
            board_[r][c] = AI_PLAYER;
            if (checkWin(r, c, AI_PLAYER)) 
            {
                // board_[r][c] = AI_PLAYER; // 恢复棋盘
                return {r, c};      // 立即获胜
            }
            board_[r][c] = EMPTY;

            // 模拟玩家落子，判断是否需要防守
            board_[r][c] = HUMAN_PLAYER;
            if (checkWin(r, c, HUMAN_PLAYER)) 
            {
                board_[r][c] = AI_PLAYER; // 恢复棋盘
                return {r, c};      // 立即防守
            }
            board_[r][c] = EMPTY;
        }
    }

    // 2. 评估每个空位的威胁程度，选择最佳防守位置
    for (int r = 0; r < BOARD_SIZE; r++) 
    {
        for (int c = 0; c < BOARD_SIZE; c++) 
        {
            if (board_[r][c] != EMPTY) continue; // 确保当前位置为空闲

            int threatLevel = evaluateThreat(r, c); // 评估威胁程度
            if (threatLevel > maxThreat) 
            {
                maxThreat = threatLevel;
                bestMove = {r, c};
            }
        }
    }

    // 3. 如果找不到威胁点，选择靠近玩家或已有棋子的空位
    if (bestMove.first == -1) 
    {
        std::vector<std::pair<int, int>> nearCells;

        for (int r = 0; r < BOARD_SIZE; r++) 
        {
            for (int c = 0; c < BOARD_SIZE; c++) 
            {
                if (board_[r][c] == EMPTY && isNearOccupied(r, c)) 
                { // 确保当前位置为空闲且靠近已有棋子
                    nearCells.push_back({r, c});
                }
            }
        }

        // 如果找到靠近已有棋子的空位，随机选择一个
        if (!nearCells.empty()) 
		{
            int num = rand();
			board_[nearCells[num % nearCells.size()].first][nearCells[num % nearCells.size()].second] = AI_PLAYER;
            return nearCells[num % nearCells.size()];
        }

        // 4. 如果所有策略都无效，选择第一个空位（保证 AI 落子）
        for (int r = 0; r < BOARD_SIZE; r++) 
        {
            for (int c = 0; c < BOARD_SIZE; c++) 
            {
                if (board_[r][c] == EMPTY) 
				{
					board_[r][c] = AI_PLAYER;
                    return {r, c}; // 返回第一个空位
                }
            }
        }
    }
	
	board_[bestMove.first][bestMove.second] = AI_PLAYER;
    return bestMove; // 返回最佳防守点或其他策略的结果
}

// 优化后的威胁评估函数
int AiGame::evaluateThreat(int r, int c, const std::string& player) 
{
    if (board_[r][c] != EMPTY) return 0;
    
    // 临时放置棋子进行评估
    board_[r][c] = player;
    
    int score = 0;
    const int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    const std::string opponent = (player == AI_PLAYER) ? HUMAN_PLAYER : AI_PLAYER;
    
    for (auto& dir : directions) 
    {
        // 计算当前方向上的连续棋子数及两端是否有空位
        int consecutive = 1;  // 当前位置的棋子
        bool blockedLeft = false;
        bool blockedRight = false;
        
        // 向左/上检查
        for (int i = 1; i <= 4; i++) 
        {
            int nr = r - i * dir[0], nc = c - i * dir[1];
            if (!isInBoard(nr, nc) || board_[nr][nc] == opponent) 
            {
                blockedLeft = true;
                break;
            }
            if (board_[nr][nc] == player) 
                consecutive++;
            else  // 空位
                break;
        }
        
        // 向右/下检查
        for (int i = 1; i <= 4; i++) 
        {
            int nr = r + i * dir[0], nc = c + i * dir[1];
            if (!isInBoard(nr, nc) || board_[nr][nc] == opponent) 
            {
                blockedRight = true;
                break;
            }
            if (board_[nr][nc] == player) 
                consecutive++;
            else  // 空位
                break;
        }
        
        // 根据连子数和是否被阻挡评分
        if (consecutive >= 5) 
        {
            score += 100000;  // 五连，必胜
        }
        else if (consecutive == 4) 
        {
            // 冲四（一端挡，一端空）或活四（两端空）
            if (!blockedLeft || !blockedRight) 
                score += (blockedLeft && blockedRight) ? 100 : 10000;
        }
        else if (consecutive == 3) 
        {
            // 活三（两端空）或冲三（一端挡）
            int openEnds = (!blockedLeft ? 1 : 0) + (!blockedRight ? 1 : 0);
            score += (openEnds == 2) ? 1000 : (openEnds == 1 ? 100 : 10);
        }
        else if (consecutive == 2) 
        {
            int openEnds = (!blockedLeft ? 1 : 0) + (!blockedRight ? 1 : 0);
            score += (openEnds == 2) ? 50 : (openEnds == 1 ? 10 : 5);
        }
        else if (consecutive == 1) 
        {
            int openEnds = (!blockedLeft ? 1 : 0) + (!blockedRight ? 1 : 0);
            score += (openEnds == 2) ? 5 : 1;
        }
    }
    
    // 恢复棋盘
    board_[r][c] = EMPTY;
    return score;
}

// 极小极大算法搜索最佳走法
int AiGame::minimax(int depth, bool isMaximizing, int alpha, int beta) 
{
    // 到达搜索深度或游戏结束
    if (depth == 0) 
    {
        int aiScore = 0, humanScore = 0;
        // 评估当前棋盘状态
        for (int i = 0; i < BOARD_SIZE; i++) 
        {
            for (int j = 0; j < BOARD_SIZE; j++) 
            {
                if (board_[i][j] == EMPTY) 
                {
                    aiScore += evaluateThreat(i, j, AI_PLAYER);
                    humanScore += evaluateThreat(i, j, HUMAN_PLAYER);
                }
            }
        }
        return aiScore - humanScore;  // AI分数减去人类分数
    }
    
    // 检查是否有玩家已经获胜
    if (checkWin(lastMove_.first, lastMove_.second, AI_PLAYER)) 
        return 100000 + depth;
    if (checkWin(lastMove_.first, lastMove_.second, HUMAN_PLAYER)) 
        return -100000 - depth;
    if (isDraw()) 
        return 0;
    
    if (isMaximizing) 
    {
        // AI回合，最大化分数
        int maxScore = -1000000;
        std::vector<std::pair<int, int>> candidates = getCandidateMoves();
        
        for (auto& move : candidates) 
        {
            int x = move.first, y = move.second;
            if (board_[x][y] != EMPTY) continue;
            
            board_[x][y] = AI_PLAYER;
            lastMove_ = {x, y};
            int score = minimax(depth - 1, false, alpha, beta);
            board_[x][y] = EMPTY;
            
            maxScore = std::max(score, maxScore);
            alpha = std::max(alpha, score);
            
            if (beta <= alpha) break;  // Alpha剪枝
        }
        return maxScore;
    } 
    else 
    {
        // 人类回合，最小化分数
        int minScore = 1000000;
        std::vector<std::pair<int, int>> candidates = getCandidateMoves();
        
        for (auto& move : candidates) 
        {
            int x = move.first, y = move.second;
            if (board_[x][y] != EMPTY) continue;
            
            board_[x][y] = HUMAN_PLAYER;
            lastMove_ = {x, y};
            int score = minimax(depth - 1, true, alpha, beta);
            board_[x][y] = EMPTY;
            
            minScore = std::min(score, minScore);
            beta = std::min(beta, score);
            
            if (beta <= alpha) break;  // Beta剪枝
        }
        return minScore;
    }
}

// 获取候选落子位置（减少搜索范围，提高效率）
std::vector<std::pair<int, int>> AiGame::getCandidateMoves() 
{
    std::vector<std::pair<int, int>> candidates;
    
    // 优先考虑已有棋子周围的位置
    for (int i = 0; i < BOARD_SIZE; i++) 
    {
        for (int j = 0; j < BOARD_SIZE; j++) 
        {
            if (board_[i][j] != EMPTY) 
            {
                // 检查周围3x3范围内的空位
                for (int dx = -2; dx <= 2; dx++) 
                {
                    for (int dy = -2; dy <= 2; dy++) 
                    {
                        int x = i + dx;
                        int y = j + dy;
                        if (isInBoard(x, y) && board_[x][y] == EMPTY) 
                        {
                            candidates.push_back({x, y});
                        }
                    }
                }
            }
        }
    }
    
    // 如果是开局，添加中心附近的位置
    if (candidates.empty()) 
    {
        for (int i = BOARD_SIZE/2 - 2; i <= BOARD_SIZE/2 + 2; i++) 
        {
            for (int j = BOARD_SIZE/2 - 2; j <= BOARD_SIZE/2 + 2; j++) 
            {
                if (isInBoard(i, j)) 
                {
                    candidates.push_back({i, j});
                }
            }
        }
    }
    
    // 去重
    std::sort(candidates.begin(), candidates.end());
    auto last = std::unique(candidates.begin(), candidates.end());
    candidates.erase(last, candidates.end());
    
    return candidates;
}

std::pair<int, int> AiGame::getBestMove()
{
    // 1. 优先检查是否能立即获胜或需要立即防守
    for (int r = 0; r < BOARD_SIZE; r++) 
    {
        for (int c = 0; c < BOARD_SIZE; c++) 
        {
            if (board_[r][c] != EMPTY) continue;

            // 检查AI是否能一步获胜
            board_[r][c] = AI_PLAYER;
            if (checkWin(r, c, AI_PLAYER)) 
            {
                board_[r][c] = EMPTY;
                return {r, c};
            }
            board_[r][c] = EMPTY;

            // 检查是否需要防守以阻止人类获胜
            board_[r][c] = HUMAN_PLAYER;
            if (checkWin(r, c, HUMAN_PLAYER)) 
            {
                board_[r][c] = EMPTY;
                return {r, c};
            }
            board_[r][c] = EMPTY;
        }
    }

    // 2. 使用极小极大算法搜索最佳走法
    std::vector<std::pair<int, int>> candidates = getCandidateMoves();
    if (candidates.empty()) 
    {
        // 如果没有候选位置，返回第一个空位（通常是游戏初期）
        for (int r = 0; r < BOARD_SIZE; r++) 
        {
            for (int c = 0; c < BOARD_SIZE; c++) 
            {
                if (board_[r][c] == EMPTY) 
                {
                    return {r, c};
                }
            }
        }
    }

    // 根据游戏阶段调整搜索深度（提高性能）
    int depth = (moveCount_ < 10) ? 2 : (moveCount_ < 20) ? 3 : 4;
    
    int bestScore = -1000000;
    std::pair<int, int> bestMove = candidates[0];
    
    for (auto& move : candidates) 
    {
        int x = move.first, y = move.second;
        if (board_[x][y] != EMPTY) continue;
        
        board_[x][y] = AI_PLAYER;
        lastMove_ = {x, y};
        int score = minimax(depth - 1, false, -1000000, 1000000);
        board_[x][y] = EMPTY;
        
        if (score > bestScore) 
        {
            bestScore = score;
            bestMove = move;
        }
    }
    
    return bestMove;
}