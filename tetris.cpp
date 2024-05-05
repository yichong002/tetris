/*
 * Version 2.0
 * Time: 20240505 19:33:30
 * support shortcut keys: 
 *     'p' - PAUSE
 *     't' - Tips for next
 *     '1,,5' - LEVEL
 * support options:
 *     's' - HxW, e.g. '-s 20x15'
 *     'l' - LEVEL
 *     't' - Tips
 * 
 * Usage:
 * Windows: x86_64-w64-mingw32-g++.exe -g tetris.cpp -o tetris.exe
 *          tetris.exe
 * Linux:   g++ -g tetris.cpp -o tetris -lncurses
 *          tetris
 */
#include <iostream>
#include <cstring>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <assert.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h> /* Sleep */

#define CHR_RIGHT 77
#define CHR_LEFT  75
#define CHR_DOWN  80
#define CHR_UP    72
#define output(txt, args...) printf(txt, ##args)

static void init_curses() {}
static void exit_curses() {}
static int timeout(int t) {return t;}
#else //linux
#include <time.h>
#include <sys/time.h>
#include <ncurses.h> /* getch */
#include <unistd.h> /* usleep */

#define CHR_RIGHT 5
#define CHR_LEFT  4
#define CHR_DOWN  2
#define CHR_UP    3
#define mingw_gettimeofday  gettimeofday
#define Sleep(x)            usleep(x * 1000)
#define output(txt, args...) printw(txt, ##args)
typedef unsigned char       uint8_t;
typedef unsigned int        uint32_t;

static int kbhit() {return 1;}
static void init_curses() {
    initscr();
    timeout(1);
    noecho();
    keypad(stdscr, 1);
}
static void exit_curses() {
	endwin();
}
#endif

static bool _tips = false;
static int _dbg = 0;
#define debug(txt, args...)  if (_dbg) output("%s[%d]: " txt "\n", __FUNCTION__, __LINE__, ##args)

#define KIND_NUM   7
#define DIRECT_NUM 4

static uint8_t HEIGHT = 20;
static uint8_t WIDTH = 15;

const int delay_list[6] = {0, 1600, 1100, 700, 400, 250};
static int level2delay(int level) {
    return (level > 0 && level < 6) ? delay_list[level] : delay_list[3];
}
static int delay2level(int delay) {
    int i;
    for (i=5; i>0; i--) if (delay <= delay_list[i]) return i;
    return 3;
}

enum {
    MOVE_NONE,
    MOVE_ROTATE, MOVE_LEFT, MOVE_RIGHT, MOVE_DOWN,
    MOVE_QUIT, MOVE_PAUSE, MOVE_HINT,
    MOVE_L1, MOVE_L2, MOVE_L3, MOVE_L4, MOVE_L5
};

enum {
    STAT_NORMAL, STAT_COLLIDE, STAT_STOP
};

// Block definition
char defBlocks [KIND_NUM][DIRECT_NUM][4][4] =
{
// Square
  {
   {
    {0, 0, 0, 0},
    {0, 1, 1, 0},
    {0, 1, 1, 0},
    {0, 0, 0, 0}
    },
   {
    {0, 0, 0, 0},
    {0, 1, 1, 0},
    {0, 1, 1, 0},
    {0, 0, 0, 0}
    },
   {
    {0, 0, 0, 0},
    {0, 1, 1, 0},
    {0, 1, 1, 0},
    {0, 0, 0, 0}
    },
   {
    {0, 0, 0, 0},
    {0, 1, 1, 0},
    {0, 1, 1, 0},
    {0, 0, 0, 0}
    }
   },

// I
  {
   {
    {0, 0, 0, 0},
    {3, 1, 2, 2},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
    },
   {
    {0, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 0, 0}
    },
   {
    {0, 0, 0, 0},
    {3, 1, 2, 2},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
    },
   {
    {0, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 0, 0},
    }
   }
  ,
// L
  {
   {
    {0, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 0, 0}
    },
   {
    {0, 0, 0, 0},
    {3, 1, 1, 0},
    {3, 0, 0, 0},
    {0, 0, 0, 0}
    },
   {
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 0, 0}
    },
   {
    {0, 0, 2, 0},
    {1, 1, 2, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
    }
   },
// L mirrored
  {
   {
    {0, 1, 0, 0},
    {0, 1, 0, 0},
    {1, 1, 0, 0},
    {0, 0, 0, 0}
    },
   {
    {1, 0, 0, 0},
    {1, 1, 2, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
    },
   {
    {0, 1, 1, 0},
    {0, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 0, 0}
    },
   {
    {0, 0, 0, 0},
    {3, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 0}
    }
   },
// N
  {
   {
    {0, 0, 1, 0},
    {0, 1, 1, 0},
    {0, 1, 0, 0},
    {0, 0, 0, 0}
    },
   {
    {0, 0, 0, 0},
    {3, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 0, 0}
    },
   {
    {0, 1, 0, 0},
    {1, 1, 0, 0},
    {1, 0, 0, 0},
    {0, 0, 0, 0}
    },
   {
    {1, 1, 0, 0},
    {0, 1, 2, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
    }
   },
// N mirrored
  {
   {
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 0}
    },
   {
    {0, 0, 0, 0},
    {0, 1, 1, 0},
    {3, 1, 0, 0},
    {0, 0, 0, 0}
    },
   {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 0, 0}
    },
   {
    {0, 1, 2, 0},
    {1, 1, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
    }
   },
// T
  {
   {
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 1, 0, 0},
    {0, 0, 0, 0}
    },
   {
    {0, 0, 0, 0},
    {3, 1, 1, 0},
    {0, 1, 0, 0},
    {0, 0, 0, 0}
    },
   {
    {0, 1, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 0, 0}
    },
   {
    {0, 1, 0, 0},
    {1, 1, 2, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
    }
   }
};

////////////////////////////////////////////////////////
class Matrix {
private:
    std::vector< std::vector<char> > data;
    char ** data_ptr = NULL;
 
public:
    Matrix(size_t rows, size_t cols) : data(rows, std::vector<char>(cols, 0)) {}
 
    size_t getRows() const {
        return data.size();
    }

    size_t getCols() const {
        return data[0].size();
    }

    void setValue(size_t row, size_t col, char value) {
        if (row < data.size() && col < data[row].size()) {
            data[row][col] = value;
        }
    }
 
    char getValue(size_t row, size_t col) const {
        if (row < data.size() && col < data[row].size()) {
            return data[row][col];
        }
        return 0; //throw exception
    }

    void reset() {
        std::fill(data[0].begin(), data[0].end(), 0);        
        std::fill(data.begin(), data.end(), data[0]);        
    }
 
    char ** allocArray() {
        if (data_ptr) return data_ptr;
        data_ptr = (char **)malloc(data.size() * sizeof(char *));
        int i;
        for (i=0; i<data.size(); i++) data_ptr[i] = data[i].data();
        return data_ptr;
    }

    void freeArray() {
        free(data_ptr);
        data_ptr = NULL;
    }
};

////////////////////////////////////////////////////////
class Block
{
public:
    Block(int type, int rotation, int pos_y, int pos_x);
    int put_data_array(char *array[], bool ignore_detail);
    void move(int action);
    void backup();
    void restore();

private:
    int m_type;
    int m_rotation;
    int m_pos_x;
    int m_pos_y;
    int b_rotation;
    int b_pos_x;
    int b_pos_y;
    char data[4][4];
};

Block::Block(int type, int rotation, int pos_y, int pos_x) {
    if (type < 0 || type >= KIND_NUM) return;
    if (rotation < 0 || rotation >= DIRECT_NUM) return;
    m_type = type;
    m_rotation = rotation;
    m_pos_y = pos_y;
    m_pos_x = pos_x;
    b_rotation = b_pos_x = b_pos_y = -1;
    memcpy(data, defBlocks[type][rotation], sizeof(data));
    debug("type %d, rotation %d, pos_y %d, pos_x %d", type, rotation, pos_y, pos_x);
}

int Block::put_data_array(char *array[], bool ignore_detail) {
    int failure = 0;
    size_t row, col;
    for (row = 0; row < 4; row++) {
        for (col = 0; col < 4; col++) {
            if (!data[row][col]) continue;
            if ((m_pos_y + row) >= HEIGHT) continue;
            if ((m_pos_x + col) < 0 || (m_pos_x + col) >= WIDTH) continue;

            char *ptr = &array[m_pos_y + row][m_pos_x + col];
            *ptr += ignore_detail ? 1 : data[row][col];
            if (ignore_detail && (*ptr > 1)) {
                *ptr = 1;
                failure = 1;
            }
        }
    }
    return failure;
}

void Block::move(int action) {
    switch (action) {
        case MOVE_LEFT:
            m_pos_x -= 1;
            return;
        case MOVE_RIGHT:
            m_pos_x += 1;
            return;
        case MOVE_DOWN:
            m_pos_y += 1;
            return;
        case MOVE_ROTATE:
            m_rotation += 1;
            break;
        case MOVE_NONE:
        default:
            return;
    }
    m_rotation %= DIRECT_NUM;
    memcpy(data, defBlocks[m_type][m_rotation], sizeof(data));
}

void Block::backup() {
    b_rotation = m_rotation;
    b_pos_x = m_pos_x;
    b_pos_y = m_pos_y;
}

void Block::restore() {
    m_rotation = b_rotation;
    m_pos_x = b_pos_x;
    m_pos_y = b_pos_y;
    memcpy(data, defBlocks[m_type][m_rotation], sizeof(data));
    b_rotation = b_pos_x = b_pos_y = -1;
}

////////////////////////////////////////////////////////
class Board
{
    enum {
        POS_FREE, POS_FILLED, POS_FILLED_2, POS_FILLED_3, POS_BORDER
    };

public:
    Board(char ch = 0);
    ~Board();
    void put_data_array(char *array[]);
    void new_block();
    void free_block();
    int check_block_data(char *array[], bool rotate);
    int move_block(int action);
    int clear_line();
    void clear_screen();
    void refresh_screen(bool clear = true);
    void set_game_pause();
    bool is_game_pause();
    void set_game_over();
    bool is_game_over();

    int delay_ms; /* milliseconds */

private:
    Block *p_block = NULL;
    int score;
    bool isPaused;
    bool isGameOver;
    char blkCh;
    Matrix *dataM = NULL;
    int next_blk_type;
    int next_blk_rota;

    int getRandom(int min, int max);
    void dump(char * ptr[]);
};

Board::Board(char ch) {
    blkCh = ch;
    debug("Board() blkCh = %d", blkCh);
    if (!ch) return;
    delay_ms = level2delay(0);
    score = 0;
    isPaused = false;
    isGameOver = false;
    next_blk_type = next_blk_rota = -1;

    dataM = new Matrix(HEIGHT, WIDTH);

    size_t row, col;
    for (row = 0; row < HEIGHT; row++) {
        for (col = 0; col < WIDTH; col++) {
            if ((col == 0) || (col == WIDTH - 1) || (row == HEIGHT - 1)) {
                dataM->setValue(row, col, POS_BORDER);
            } else {
                dataM->setValue(row, col, POS_FREE);
            }
        }
    }
}

Board::~Board() {
    delete p_block;
    delete dataM;
    debug("~Board() dataM = %p, blkCh = %d", dataM, blkCh);
}

void Board::put_data_array(char *array[]) {
    size_t row, col;
    for (row = 0; row < HEIGHT; row++) {
        for (col = 0; col < WIDTH; col++) {
            array[row][col] += dataM->getValue(row, col);
        }
    }
}

void Board::dump(char * ptr[]) {
    size_t row, col;
    for (row = 0; row < HEIGHT; row++) {
        for (col = 0; col < WIDTH; col++) {
            debug("data[%d][%d] = %d", row, col, ptr ? ptr[row][col] : dataM->getValue(row, col));
        }
    }
}

int Board::getRandom(int min, int max) {
    return rand() % (max - min + 1) + min;
}

void Board::new_block() {
    int type = next_blk_type;
    int rotation = next_blk_rota;
    if (!p_block) {
        if (type < 0) type = getRandom(0, 6);
        if (rotation < 0) rotation = getRandom(0, 3);
        int pos_x = 0;
        int pos_y = WIDTH/2-2;
        p_block = new Block(type, rotation, pos_x, pos_y);
    }
    next_blk_type = getRandom(0, 6);
    next_blk_rota = getRandom(0, 3);
}

void Board::free_block() {
    if (!p_block) {
        return;
    }
/*
    char *data;
    int data_size = HEIGHT * WIDTH * sizeof(*data);
    data = malloc(data_size);
    memset(data, 0, data_size);
    char (*array)[];
    array = malloc(HEIGHT * sizeof(*array));
    for (size_t i = 0; i<HEIGHT; i++) {
        array[i] = &data[i * WIDTH];
    }
*/
    Matrix mat(HEIGHT, WIDTH);
    char **array = mat.allocArray();

    p_block->put_data_array(array, true);

    size_t row, col;
    for (row = 0; row < HEIGHT - 1; row++) {
        for (col = 1; col < WIDTH - 1; col++) {
            if (array[row][col]) dataM->setValue(row, col, POS_FILLED);
        }
    }
    delete p_block;
    p_block = NULL;
/*
    free(data);
    free(array);
*/
    mat.freeArray();
}

/* return 0: success, 2: right-collided, 3: left-collided, -1: failure */
int Board::check_block_data(char *array[], bool rotate) {
    int collided = 0;
    size_t row, col;
    for (row = 0; row < HEIGHT; row++) {
        for (col = 0; col < WIDTH; col++) {
            if (dataM->getValue(row, col) && array[row][col]) {
                if (array[row][col] == 1) return -1;
                if (!collided) collided = rotate ? array[row][col] : 1;
                if (collided == array[row][col]) continue;
                return -1;
            }
        }
    }
    return collided;
}

int Board::move_block(int action) {
    if (!p_block) {
        return STAT_NORMAL;
    }

    int collide;
    bool is_rotate = (action == MOVE_ROTATE);
    //char array[HEIGHT][WIDTH];
    Matrix mat(HEIGHT, WIDTH);
    char **array = mat.allocArray();

    p_block->backup();
    do {
        p_block->move(action);
        //memset(array, 0, sizeof(array));
        mat.reset();
        p_block->put_data_array(array, false);
        collide = check_block_data(array, is_rotate);
        if (collide == POS_FILLED_2) action = MOVE_LEFT;
        if (collide == POS_FILLED_3) action = MOVE_RIGHT;
    } while(collide > 1);

    mat.freeArray();

    if (collide < 0) {
        p_block->restore();
        switch (action) {
        case MOVE_DOWN:
            return STAT_STOP;
        case MOVE_LEFT:
        case MOVE_RIGHT:
        case MOVE_ROTATE:
            return STAT_COLLIDE;
        default:
            break;
        }
    }
    return STAT_NORMAL;
}

int Board::clear_line() {
    int index, clear_lines = 0;
    size_t row, col;

    for (index = HEIGHT - 2; index >= 0; index--) {
        for (col = 1; col < WIDTH - 1; col++) {
            if (dataM->getValue(index, col) == POS_FREE) {
                break;
            }
        }
        if (col < WIDTH - 1) continue;

        // Moves all the upper lines one row down
        for (row = index; row > 0; row--) {
            for (col = 1; col < WIDTH - 1; col++) {
                dataM->setValue(row, col, dataM->getValue(row-1, col));
            }
        }
        clear_lines ++;
        index ++;
        if (clear_lines > 1) continue;
        for (col = 1; col < WIDTH - 1; col++) {
            dataM->setValue(0, col, POS_FREE);
        }
    }
    score += clear_lines;
    return clear_lines;
}

void Board::clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    clear();
#endif
}

void Board::refresh_screen(bool clear) {
    //char array[HEIGHT][WIDTH];
    //memset(array, 0, sizeof(array));
    Matrix mat(HEIGHT, WIDTH);
    char **array = mat.allocArray();
    mat.reset();

    put_data_array(array);

    if (p_block) {
        isGameOver = p_block->put_data_array(array, true);
    }

    if (clear) clear_screen();

    bool std_output = false;
#ifdef _WIN32
    std_output = true;
#else
    if (!clear) std_output = true;
#endif

    char title[] = "    Tetris Speed ";
    if (std_output)
        std::cout << title << delay2level(delay_ms) << std::endl;
    else
        output("%s%d\n", title, delay2level(delay_ms));

    char tips_buffer[4 + 8 + 1] = {0};
    char buffer[50 * 2 + 1] = {0};
    assert(WIDTH <= 50);

    size_t t = next_blk_type, r = next_blk_rota, i, j;
    for (i = 0; i < HEIGHT; i++) {
        for (j = 0; j < WIDTH; j++) {
            switch (array[i][j]) {
            case POS_FREE: //Empty
                //std::cout << "  ";
                buffer[j * 2] = buffer[j * 2 + 1] = ' ';
                break;
            case POS_FILLED: //Block
                //std::cout << blkCh << blkCh;
                buffer[j * 2] = buffer[j * 2 + 1] = blkCh;
                break;
            case POS_BORDER: //Border
                //std::cout << "$$";
                buffer[j * 2] = buffer[j * 2 + 1] = '$';
                break;
            }
        }
        if (_tips && (i < 4) && (t < KIND_NUM) && (r < DIRECT_NUM)) {
            for (j = 0; j < 4; j++) {tips_buffer[j] = ' ';}
            for (j = 0; j < 4; j++) {tips_buffer[4 + j*2] = tips_buffer[5 + j*2] = defBlocks[t][r][i][j] ? blkCh:' ';}
            tips_buffer[12] = '\0';
        } else {
            tips_buffer[0] = '\0';
        }

        if (std_output)
            std::cout << buffer << tips_buffer << std::endl;
        else
            output("%s%s\n", buffer, tips_buffer);
    }
    if (std_output)
        std::cout << "    Score: " << score << std::endl;
    else
        output("    Score: %d\n", score);

    mat.freeArray();
}

void Board::set_game_pause() {
    isPaused = !isPaused;
}

bool Board::is_game_pause() {
    return isPaused;
}

void Board::set_game_over() {
    isGameOver = !isGameOver;
}

bool Board::is_game_over() {
    return isGameOver;
}

////////////////////////////////////////////////////////
class Frame
{
public:
    Frame(int delay, char ch);
    ~Frame(){delete m_board;}
    void start();
    void print_result();
    int get_user_input();
    bool is_timeout(int delay);
    void reset_timer();

private:
    Board *m_board = NULL;
    struct timeval m_timer;
};

Frame::Frame(int delay, char ch) {
    if (!m_board) m_board = new Board(ch);
    m_board->delay_ms = delay;
}

void Frame::start() {
    int action, result;

    reset_timer();
    m_board->new_block();
    while (m_board->is_game_over() == false) {

        action = MOVE_NONE;

        Sleep(1); /* sleep a while */
        if (_dbg) Sleep(100);

        if (kbhit()) { //kbhit returns a non-zero integer if a key is in the keyboard buffer.
            action = get_user_input();
            if (action >= MOVE_L1 && action <= MOVE_L5) {
                m_board->delay_ms = level2delay(action + 1 - MOVE_L1);
                continue;
            }
            if (MOVE_HINT == action) {
                _tips = !_tips;
                continue;
            }
            if (MOVE_QUIT == action) {
                m_board->set_game_over();
                continue;
            }
            if (MOVE_PAUSE == action) {
                m_board->set_game_pause();
            }
            else if (m_board->is_game_pause() && action >= MOVE_ROTATE && action <= MOVE_DOWN) {
                m_board->set_game_pause();
            }
        }
        if (is_timeout(m_board->delay_ms)) {
            action = MOVE_DOWN;
        }
        if (m_board->is_game_pause()) {
            action = MOVE_NONE;
        }
        if (MOVE_NONE == action) {
            continue;
        }
        if (MOVE_DOWN == action) {
            reset_timer();
        }
        result = m_board->move_block(action);
        if (result == STAT_STOP) {
            m_board->free_block();
            m_board->clear_line();
            m_board->refresh_screen();
            m_board->new_block();
            m_board->move_block(MOVE_NONE); /* used to check game-over */
        }
        m_board->refresh_screen();
    }
}

void Frame::print_result() {
    m_board->refresh_screen(false);
}
int Frame::get_user_input() {
    char key;
    key = getch();

    switch (key) {
    case CHR_RIGHT:
    case 'd': //right
        return MOVE_RIGHT;
    case CHR_LEFT:
    case 'a': //left
        return MOVE_LEFT;
    case CHR_DOWN:
    case 's': //down
        return MOVE_DOWN;
    case CHR_UP:
    case 'w': //rotate
        return MOVE_ROTATE;
    case 't': //hint
        return MOVE_HINT;
    case 'p': //pause
        return MOVE_PAUSE;
    case 'q': //quit
        return MOVE_QUIT;
    case '1': //Level-1
        return MOVE_L1;
    case '2': //Level-2
        return MOVE_L2;
    case '3': //Level-3
        return MOVE_L3;
    case '4': //Level-4
        return MOVE_L4;
    case '5': //Level-5
        return MOVE_L5;
    }
    return MOVE_NONE;
}

bool Frame::is_timeout(int delay) {
    struct timeval now;
    mingw_gettimeofday(&now, NULL);
    return ((now.tv_sec - m_timer.tv_sec)*1000L + (now.tv_usec - m_timer.tv_usec)/1000L) > delay;
}

void Frame::reset_timer() {
    mingw_gettimeofday(&m_timer, NULL);
}

////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    int level, delay = level2delay(0), help = 0;
    char c, block_ch = 177;
    int height = 0, width = 0, x;
    std::string str;
    while ((c = getopt(argc, argv, "dhtl:c:s:")) != -1) {
        switch (c) {
        case 'l':
            level = (uint32_t) atoi(optarg);
            if (level < 1 || level > 5) help = 1;
            delay = level2delay(level);
            break;
        case 'c':
            block_ch = (char) atoi(optarg);
            if (!block_ch) help = 1;
            break;
        case 's':
            str = optarg;
            x = str.find("x");
            if (x != (int)std::string::npos) {
                height = atoi(str.substr(0, x).c_str());
                width = atoi(str.substr(x + 1).c_str());
                debug("height = %d, width = %d", height, width);
            }
            if (height >= 10 && height <= 50) HEIGHT = height;
            else help = 1;
            if (width >= 8 && width <= 40) WIDTH = width;
            else help = 1;
            break;
        case 'd':
            _dbg = 1;
            break;
        case 't':
            _tips = true;
            break;
        case 'h':
        default:
            help = 1;
        }
    }

    if (help) {
        std::cout << argv[0] << " [-s HxW] [-l level] [-c char] [-t]\n"
                                "  size:  \theight[10, 50], width[8, 40], default 20x15\n"
                                "  level: \t[1, 5] is supported, default 3\n"
                                "  char:  \tblock shape char, default 177\n"
                                "  tips:  \tenable preview of next block\n";
        exit(0);
    }

    srand(time(NULL));
    init_curses();

    Frame m_frame(delay, block_ch);
    m_frame.start(); //start game

    exit_curses();
#ifndef _WIN32
    m_frame.print_result(); //end game
#endif

    std::cout << "===============\n"
                 "~~~Game Over~~~\n"
                 "===============\n";
    exit(0);
}
