#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif
#include <SDL2/SDL.h>
#ifndef __EMSCRIPTEN__
#include <SDL2_image/SDL_image.h>
#else
#include <SDL2/SDL_image.h>
#endif
#include <vector>
#include <iostream>
#include <string>
using namespace std;

enum Rotation {
    UP,
    RIGHT,
    DOWN,
    LEFT
};

enum CellType {
    EMPTY,
    DEFAULT,
    ARROW
};

struct Cell {
    Rotation rotation;
    CellType type;

    Cell(CellType type) {
        this->rotation = UP;
        this->type = type;
    }
};

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;
const int CELL_SIZE = 10;

const int GRID_WIDTH = SCREEN_WIDTH / CELL_SIZE;
const int GRID_HEIGHT = SCREEN_HEIGHT / CELL_SIZE;
vector<vector<Cell>> grid(GRID_HEIGHT, vector<Cell>(GRID_WIDTH, Cell(EMPTY)));
vector<SDL_Texture*> textures;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

bool running = true;
int frames = 0;

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return false;
    }
    if (IMG_Init(IMG_INIT_PNG) != 2) {
        SDL_Log("IMG_Init Error: %s", SDL_GetError());
        return false;
    }
    window = SDL_CreateWindow("Game of Life", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool initTextures() {
    vector<string> imagePaths = {"img/empty.png", "img/cell.png", "img/arrow.png"};
    bool success = true;
    for (int i = 0; i < imagePaths.size(); i++) {
        SDL_Texture* texture = IMG_LoadTexture(renderer, imagePaths[i].c_str());
        if (texture == NULL) {
            // We still want to let to user know about all the errors that
            // have happened, and not have to play error Whack-a-Mole. But
            // at least it's way cheaper than the carnival version.
            SDL_Log("Failed to load image: %s ", imagePaths[i].c_str());
            success = false;
            continue;
        }
        textures.push_back(texture);
    }

    return success;
}

void randomizeGrid() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x] = Cell(static_cast<CellType> (rand() % 3));
        }
    }
}

void updateGrid() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            switch (grid[y][x].type) {
            case ARROW: {
                Cell cell = grid[y][x];
                int dx = 0, dy = 0;
                if (cell.rotation == 0) dy = -1;
                if (cell.rotation == 1) dx = 1;
                if (cell.rotation == 2) dy = 1;
                if (cell.rotation == 3) dx = -1;

                int nx = x + dx, ny = y + dy;

                // Traverse the line of cells to find the last empty space
                vector<pair<int, int>> cellsToMove;
                while (nx >= 0 && ny >= 0 && nx < GRID_WIDTH && ny < GRID_HEIGHT) {
                    if (grid[ny][nx].type == 0) {
                        break; // Found an empty space
                    }
                    cellsToMove.push_back({nx, ny});
                    nx += dx;
                    ny += dy;
                }

                // If there’s space to move, shift all cells forward
                if (nx >= 0 && ny >= 0 && nx < GRID_WIDTH && ny < GRID_HEIGHT && grid[ny][nx].type == 0) {
                    // Move cells in reverse order to avoid overwriting
                    for (int i = cellsToMove.size() - 1; i >= 0; --i) {
                        int cx = cellsToMove[i].first;
                        int cy = cellsToMove[i].second;
                        grid[cy + dy][cx + dx] = grid[cy][cx];
                    }
                    // Move the pusher into the first position
                    grid[y + dy][x + dx] = cell;
                    // Clear the pusher's original position
                    grid[y][x] = Cell(EMPTY);
                }
            }; break;
            default:
                break;
            }
        }
    }
}

void renderGrid() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            SDL_Rect cell = {x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
            if (grid[y][x].type == EMPTY) continue;
            SDL_Texture* texture = textures[grid[y][x].type];
            SDL_RenderCopyEx(renderer, texture, NULL, &cell, grid[y][x].rotation * 90, NULL, SDL_FLIP_NONE);
        }
    }

    SDL_RenderPresent(renderer);
}

void gameLoop() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
            break;
        }
        frames++;
        if (frames % 10 == 0) {
            updateGrid();
        }
        renderGrid();
    }
}

void cleanupSDL() {
    for (SDL_Texture* texture : textures) {
        SDL_DestroyTexture(texture);
    }
    textures.clear();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main() {
    if (!initSDL()) return -1;
    if (!initTextures()) return -1;

    randomizeGrid();
    #ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(gameLoop, 0, 1);
    #else
    while (running) {
        Uint32 frameStart = SDL_GetTicks();

        gameLoop();
        
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < 50) {
            SDL_Delay(50 - frameTime);
        }
    }
    #endif

    cleanupSDL();
    return 0;
}
