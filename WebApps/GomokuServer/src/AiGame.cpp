#include "AiGame.h"

#include <chrono>
#include <thread>
#include <algorithm>
#include <unordered_set>
#include <climits>

// 评分权重 - 平衡攻防
const int SCORE_AI_FIVE = 1000000;      // AI五连
const int SCORE_AI_FOUR = 100000;       // AI活四
const int SCORE_AI_BLOCKED_FOUR = 10000;// AI冲四
const int SCORE_AI_THREE = 2000;        // AI活三
const int SCORE_AI_TWO = 100;           // AI活二

const int SCORE_PLAYER_FIVE = 1000000;  // 玩家五连
const int SCORE_PLAYER_FOUR = 20000;    // 玩家活四
const int SCORE_PLAYER_BLOCKED_FOUR = 15000; // 玩家冲四
const int SCORE_PLAYER_THREE = 8000;    // 玩家活三（远高于AI）
const int SCORE_PLAYER_TWO = 300;       // 玩家活二

// 方向数组：水平、垂直、对角线、反对角线
const int DIRECTIONS[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

// 优化：使用更高效的数据结构和算法
AiGame::AiGame(int userId)
    : gameOver_(false)
    , userId_(userId)
    , moveCount_(0)
    , lastMove_(-1, -1)
    , board_(BOARD_SIZE, std::vector<std::string>(BOARD_SIZE, EMPTY))
{
    srand(time(0));
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

// 优化：简化胜利检查
bool AiGame::checkWin(int x, int y, const std::string& player) 
{
    const int dx[] = {1, 0, 1, 1};
    const int dy[] = {0, 1, 1, -1};
    
    for (int dir = 0; dir < 4; dir++) 
    {
        int count = 1;
        
        // 单一方向检查（合并正反）
        for (int sign = -1; sign <= 1; sign += 2) {
            for (int i = 1; i < 5; i++) 
            {
                int newX = x + sign * dx[dir] * i;
                int newY = y + sign * dy[dir] * i;
                if (!isInBoard(newX, newY)) break;
                if (board_[newX][newY] != player) break;
                count++;
            }
        }
        
        if (count >= 5) return true;
    }
    return false;
}

std::vector<std::pair<int, int>> AiGame::getCandidateMoves() 
{
    std::unordered_set<long long> uniqueMoves;
    std::vector<std::pair<int, int>> candidates;
    
    // 检查整个棋盘，只关注有棋子周围的空位
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (board_[r][c] == EMPTY) continue;
            
            // 在2格范围内搜索候选位置
            for (int dr = -2; dr <= 2; dr++) {
                for (int dc = -2; dc <= 2; dc++) {
                    if (dr == 0 && dc == 0) continue;
                    
                    int nr = r + dr;
                    int nc = c + dc;
                    
                    if (isInBoard(nr, nc)) {
                        if (board_[nr][nc] == EMPTY) {
                            long long key = static_cast<long long>(nr) * BOARD_SIZE + nc;
                            if (uniqueMoves.insert(key).second) {
                                candidates.emplace_back(nr, nc);
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 开局处理：没有候选点时选择中心
    if (candidates.empty()) {
        candidates.emplace_back(BOARD_SIZE/2, BOARD_SIZE/2);
    }
    
    return candidates;
}

int AiGame::evaluateThreat(int r, int c, const std::string& player) 
{
    if (board_[r][c] != EMPTY) return 0;
    
    const std::string opponent = (player == AI_PLAYER) ? HUMAN_PLAYER : AI_PLAYER;
    bool isPlayer = (player == HUMAN_PLAYER);
    int totalScore = 0;
    
    // 临时设置棋子以评估威胁
    board_[r][c] = player;
    
    for (const auto& dir : DIRECTIONS) {
        int playerCount = 1;  // 当前落子位置
        int openEnds = 2;     // 初始两端开放
        
        // 检查两个方向
        for (int sign = -1; sign <= 1; sign += 2) {
            bool blocked = false;
            int consecutive = 0;
            
            for (int step = 1; step <= 4; step++) {
                int nr = r + sign * step * dir[0];
                int nc = c + sign * step * dir[1];
                
                if (!isInBoard(nr, nc)) {
                    openEnds--;
                    blocked = true;
                    break;
                }
                
                if (board_[nr][nc] == player) {
                    consecutive++;
                    playerCount++;
                } 
                else if (board_[nr][nc] == opponent) {
                    openEnds--;
                    blocked = true;
                    break;
                } 
                else { // 空位
                    break;
                }
            }
            
            if (blocked) continue;
        }
        
        // 根据连子数和开放度评分
        int lineScore = 0;
        
        // 五连检查
        if (playerCount >= 5) {
            lineScore = isPlayer ? SCORE_PLAYER_FIVE : SCORE_AI_FIVE;
        } 
        // 四连
        else if (playerCount == 4) {
            if (openEnds == 2) {
                lineScore = isPlayer ? SCORE_PLAYER_FOUR : SCORE_AI_FOUR;
            } else if (openEnds == 1) {
                lineScore = isPlayer ? SCORE_PLAYER_BLOCKED_FOUR : SCORE_AI_BLOCKED_FOUR;
            }
        }
        // 三连
        else if (playerCount == 3) {
            if (openEnds == 2) {
                lineScore = isPlayer ? SCORE_PLAYER_THREE : SCORE_AI_THREE;
            }
        }
        // 二连
        else if (playerCount == 2) {
            if (openEnds == 2) {
                lineScore = isPlayer ? SCORE_PLAYER_TWO : SCORE_AI_TWO;
            }
        }
        
        totalScore += lineScore;
        
        // 玩家高威胁提前返回
        if (isPlayer && lineScore >= SCORE_PLAYER_THREE) {
            board_[r][c] = EMPTY;
            return totalScore;
        }
    }
    
    board_[r][c] = EMPTY;
    return totalScore;
}

int AiGame::evaluateBoard() 
{
    int aiScore = 0;
    int playerScore = 0;
    
    // 只评估候选位置
    auto candidates = getCandidateMoves();
    for (const auto& move : candidates) {
        // 评估AI进攻机会
        aiScore += evaluateThreat(move.first, move.second, AI_PLAYER);
        // 评估玩家威胁
        playerScore += evaluateThreat(move.first, move.second, HUMAN_PLAYER);
    }
    
    // 平衡攻防：进攻分数 + 防守分数（玩家威胁的负值）
    return aiScore * 0.7 - playerScore * 1.3;
}

int AiGame::minimax(int depth, bool isMaximizing, int alpha, int beta) 
{
    // 叶子节点或终止状态
    if (depth == 0) {
        return evaluateBoard();
    }
    
    // 胜负检查（提前终止）
    if (checkWin(lastMove_.first, lastMove_.second, AI_PLAYER)) 
        return SCORE_AI_FIVE + depth * 1000;
    if (checkWin(lastMove_.first, lastMove_.second, HUMAN_PLAYER)) 
        return -SCORE_PLAYER_FIVE - depth * 1000;
    if (isDraw()) return 0;
    
    auto candidates = getCandidateMoves();
    if (candidates.empty()) return 0;
    
    // 按威胁程度排序候选点
    std::sort(candidates.begin(), candidates.end(), [&](const auto& a, const auto& b) {
        // 优先考虑玩家威胁（防守）
        int threatA = evaluateThreat(a.first, a.second, HUMAN_PLAYER);
        int threatB = evaluateThreat(b.first, b.second, HUMAN_PLAYER);
        if (threatA != threatB) return threatA > threatB;
        
        // 其次考虑AI进攻机会
        int aiScoreA = evaluateThreat(a.first, a.second, AI_PLAYER);
        int aiScoreB = evaluateThreat(b.first, b.second, AI_PLAYER);
        return aiScoreA > aiScoreB;
    });
    
    // 限制候选点数量以提高效率
    if (candidates.size() > 10) {
        candidates.resize(10);
    }
    
    if (isMaximizing) {  // AI回合（最大化分数）
        int maxEval = INT_MIN;
        for (const auto& move : candidates) {
            int r = move.first, c = move.second;
            board_[r][c] = AI_PLAYER;
            lastMove_ = move;
            
            int eval = minimax(depth - 1, false, alpha, beta);
            board_[r][c] = EMPTY;
            
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) break;  // Alpha-Beta剪枝
        }
        return maxEval;
    } 
    else {  // 玩家回合（最小化分数）
        int minEval = INT_MAX;
        for (const auto& move : candidates) {
            int r = move.first, c = move.second;
            board_[r][c] = HUMAN_PLAYER;
            lastMove_ = move;
            
            int eval = minimax(depth - 1, true, alpha, beta);
            board_[r][c] = EMPTY;
            
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) break;  // Alpha-Beta剪枝
        }
        return minEval;
    }
}

std::pair<int, int> AiGame::getBestMove() 
{
    // 1. 检查必胜位置（AI或玩家）
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (board_[r][c] != EMPTY) continue;
            
            // 检查AI胜利
            board_[r][c] = AI_PLAYER;
            if (checkWin(r, c, AI_PLAYER)) {
                board_[r][c] = EMPTY;
                return {r, c};
            }
            board_[r][c] = EMPTY;
            
            // 检查玩家威胁（必须防守）
            board_[r][c] = HUMAN_PLAYER;
            if (checkWin(r, c, HUMAN_PLAYER)) {
                board_[r][c] = EMPTY;
                return {r, c};
            }
            board_[r][c] = EMPTY;
        }
    }
    
    // 2. 获取候选位置
    auto candidates = getCandidateMoves();
    if (candidates.empty()) {
        return {BOARD_SIZE/2, BOARD_SIZE/2};
    }
    
    // 3. 按威胁程度排序候选点（玩家威胁优先）
    std::sort(candidates.begin(), candidates.end(), [&](const auto& a, const auto& b) {
        // 优先玩家威胁（防守）
        int threatA = evaluateThreat(a.first, a.second, HUMAN_PLAYER);
        int threatB = evaluateThreat(b.first, b.second, HUMAN_PLAYER);
        if (threatA != threatB) return threatA > threatB;
        
        // 其次AI进攻机会
        int aiScoreA = evaluateThreat(a.first, a.second, AI_PLAYER);
        int aiScoreB = evaluateThreat(b.first, b.second, AI_PLAYER);
        return aiScoreA > aiScoreB;
    });
    
    // 4. 动态调整搜索深度
    int maxDepth = 3;  // 基础深度
    if (moveCount_ > 20) maxDepth = 4;  // 中局增加深度
    if (candidates.size() > 5) {
        candidates.resize(5);  // 限制候选点数量
    }
    
    // 5. 搜索最佳移动
    std::pair<int, int> bestMove = candidates[0];
    int bestScore = INT_MIN;
    
    for (const auto& move : candidates) {
        int r = move.first, c = move.second;
        board_[r][c] = AI_PLAYER;
        lastMove_ = move;
        
        int score = minimax(maxDepth, false, INT_MIN, INT_MAX);
        board_[r][c] = EMPTY;
        
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            
            // 发现高威胁及时响应
            if (evaluateThreat(r, c, HUMAN_PLAYER) >= SCORE_PLAYER_THREE) {
                break;
            }
        }
    }
    
    return bestMove;
}